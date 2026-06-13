/* NEON DESCENT — M1+M2+M3+M4 (no-CRT Win32 + OpenGL 즉시모드)
 * rules/10·20·30. DESIGN §5·§6·§8·§10·§11·§13·§14·§17·§18·§21.
 * M4: 상태머신(TITLE/PLAYING/PAUSE/GAMEOVER), 3x5 폰트, HUD, 점수/BITS,
 *     대시(이동기)+잔상, juice(셰이크·히트스톱·플래시·넉백·텔레그래프).
 */
#include <windows.h>
#include <gl/gl.h>
#include <xmmintrin.h>

int _fltused = 0;

#pragma function(memset)
void *memset(void *d, int v, size_t n){ unsigned char *p=(unsigned char*)d; while(n--) *p++=(unsigned char)v; return d; }

static float f_sqrt(float x){ return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x))); }
#define PI 3.14159265f
static float f_sin(float x){ /* fast sine, wrap to [-PI,PI] */
  while(x> PI) x-=2.0f*PI; while(x<-PI) x+=2.0f*PI;
  float y=1.27323954f*x-0.405284735f*x*(x<0.0f?-x:x);
  return 0.225f*(y*(y<0.0f?-y:y)-y)+y;
}
static float f_cos(float x){ return f_sin(x+1.57079633f); }
typedef struct { float x, y; } vec2;

/* sim RNG(결정론) + fx RNG(비주얼 전용 — sim 결정론 보호) */
static unsigned int g_rng=2463534242u, g_master=1u, g_rngFx=88675123u;
static unsigned int xrnd(void){ unsigned int x=g_rng; x^=x<<13; x^=x>>17; x^=x<<5; g_rng=x; return x; }
static float rnd01(void){ return (float)(xrnd()&0xFFFFFF)/(float)0xFFFFFF; }
static float rndsym(void){ return rnd01()*2.0f-1.0f; }
static float fxsym(void){ unsigned int x=g_rngFx; x^=x<<13; x^=x>>17; x^=x<<5; g_rngFx=x; return (float)(x&0xFFFFFF)/(float)0x7FFFFF-1.0f; }

static unsigned char g_keys[256], g_kpress[256];
static int g_mouseX, g_mouseY, g_mouseDown, g_mousePressed, g_quit, g_winW=1280, g_winH=720;

/* ---- 사운드 (§16 — waveOut 44100Hz 모노 16bit, 실시간 신스, 샘플 0개) ---- */
#define SND_RATE 44100
#define SND_BUFS 4
#define SND_SAMP 735
#define MAXVOICE 16
#define SFX_SHOOT 0
#define SFX_HIT 1
#define SFX_DEATH 2
#define SFX_HURT 3
#define SFX_PICKUP 4
#define SFX_DASH 5
#define SFX_COIN 6
#define SFX_DOOROPEN 7
#define SFX_DOORCLOSE 8
#define SFX_PHASE 9
#define SFX_BOSSDIE 10
typedef struct { int active, id; unsigned int t, rng; float ph, ph2, ph3, lp, jit; } Voice;
static HWAVEOUT g_wo;
static WAVEHDR g_whdr[SND_BUFS];
static short g_wbuf[SND_BUFS][SND_SAMP];
static int g_sndOn, g_mute;
static unsigned int g_sclock, g_lastSfx[12];
static Voice g_voice[MAXVOICE];

static float env_ad(float ts,float a,float L){ /* 어택 + 의사-지수 감쇠(이차) */
  if(ts<a) return ts/a;
  float u=1.0f-(ts-a)/(L-a); if(u<0.0f)u=0.0f; return u*u;
}
static float vnoise(Voice*v){ unsigned int x=v->rng; x^=x<<13; x^=x>>17; x^=x<<5; v->rng=x; return (float)(x&0xFFFFFF)/(float)0x7FFFFF-1.0f; }
static float osc_sin(float ph){ return f_sin(ph*6.2831853f); }
static float osc_sq(float ph){ return ph<0.5f?1.0f:-1.0f; }
static float osc_saw(float ph){ return 2.0f*ph-1.0f; }
static void vstep(float*ph,float f){ *ph+=f/(float)SND_RATE; if(*ph>=1.0f)*ph-=1.0f; }

static float voice_sample(Voice*v){
  float ts=(float)v->t/(float)SND_RATE, s=0.0f;
  v->t++;
  switch(v->id){
    case SFX_SHOOT:{ if(ts>0.06f){v->active=0;break;}
      float f=(700.0f-300.0f*(ts/0.06f))*v->jit;
      vstep(&v->ph,f); s=osc_sq(v->ph)*0.18f*env_ad(ts,0.002f,0.06f); }break;
    case SFX_HIT:{ if(ts>0.05f){v->active=0;break;}
      s=vnoise(v)*0.15f*env_ad(ts,0.001f,0.05f);
      if(ts<0.001f)s+=0.3f; }break;
    case SFX_DEATH:{ if(ts>0.18f){v->active=0;break;}
      float f=300.0f-220.0f*(ts/0.18f);
      vstep(&v->ph,f);
      s=(osc_sq(v->ph)*0.22f+vnoise(v)*0.10f)*env_ad(ts,0.002f,0.18f); }break;
    case SFX_HURT:{ if(ts>0.25f){v->active=0;break;}
      float f=120.0f-60.0f*(ts/0.25f);
      vstep(&v->ph,f);
      s=(osc_sin(v->ph)*0.35f+vnoise(v)*0.30f)*env_ad(ts,0.001f,0.25f); }break;
    case SFX_PICKUP:{ if(ts>0.15f){v->active=0;break;}
      int note=(int)(ts/0.05f); if(note>2)note=2;
      float f=note==0?440.0f:(note==1?554.0f:659.0f), tn=ts-(float)note*0.05f;
      vstep(&v->ph,f); s=osc_sin(v->ph)*0.20f*env_ad(tn,0.005f,0.05f); }break;
    case SFX_DASH:{ if(ts>0.12f){v->active=0;break;}
      float u=ts/0.12f, n=vnoise(v);
      v->lp+=(n-v->lp)*(0.10f+0.55f*u); /* 대역 상승 whoosh */
      s=v->lp*0.30f*f_sin(PI*u)*2.0f*0.15f*6.0f; if(s>0.15f)s=0.15f; if(s<-0.15f)s=-0.15f; }break;
    case SFX_COIN:{ if(ts>0.05f){v->active=0;break;}
      float f=880.0f+440.0f*(ts/0.05f)*2.0f;
      vstep(&v->ph,f); s=osc_sin(v->ph)*0.12f*env_ad(ts,0.001f,0.05f); }break;
    case SFX_DOOROPEN:{ if(ts>0.30f){v->active=0;break;}
      float f=80.0f+120.0f*(ts/0.30f);
      vstep(&v->ph,f); s=osc_sin(v->ph)*0.18f*env_ad(ts,0.01f,0.30f); }break;
    case SFX_DOORCLOSE:{ if(ts>0.30f){v->active=0;break;}
      float f=200.0f-120.0f*(ts/0.30f);
      vstep(&v->ph,f); s=osc_sin(v->ph)*0.18f*env_ad(ts,0.01f,0.30f); }break;
    case SFX_PHASE:{ if(ts>0.60f){v->active=0;break;}
      vstep(&v->ph,110.0f); vstep(&v->ph2,111.1f); vstep(&v->ph3,164.8f);
      s=(osc_saw(v->ph)+osc_saw(v->ph2)+osc_saw(v->ph3))*0.0667f*env_ad(ts,0.05f,0.60f); }break;
    case SFX_BOSSDIE:{ if(ts>0.70f){v->active=0;break;}
      float f=200.0f-160.0f*(ts/0.70f);
      vstep(&v->ph,f);
      s=(osc_saw(v->ph)*0.5f+vnoise(v)*0.25f)*env_ad(ts,0.005f,0.70f); }break;
    default: v->active=0; break;
  }
  return s;
}
static void snd_play(int id){
  int i;
  if(!g_sndOn) return;
  if(g_sclock-g_lastSfx[id]<353u) return; /* 8ms 중복 병합 */
  g_lastSfx[id]=g_sclock;
  for(i=0;i<MAXVOICE;i++) if(!g_voice[i].active){ Voice*v=&g_voice[i];
    v->active=1; v->id=id; v->t=0; v->ph=0; v->ph2=0; v->ph3=0; v->lp=0;
    v->rng=g_sclock*2654435761u+(unsigned)i+1u;
    v->jit=0.95f+(float)(v->rng&255)/255.0f*0.10f;
    return; }
}
static void snd_fill(short *buf){
  int s,i;
  for(s=0;s<SND_SAMP;s++){
    float acc=0.0f;
    for(i=0;i<MAXVOICE;i++) if(g_voice[i].active) acc+=voice_sample(&g_voice[i]);
    if(!g_mute){ acc*=0.85f; if(acc>1.0f)acc=1.0f; if(acc<-1.0f)acc=-1.0f; } else acc=0.0f;
    buf[s]=(short)(acc*32767.0f);
  }
  g_sclock+=SND_SAMP;
}
static void snd_init(void){
  static WAVEFORMATEX wfx;
  int i;
  wfx.wFormatTag=WAVE_FORMAT_PCM; wfx.nChannels=1; wfx.nSamplesPerSec=SND_RATE;
  wfx.wBitsPerSample=16; wfx.nBlockAlign=2; wfx.nAvgBytesPerSec=SND_RATE*2; wfx.cbSize=0;
  if(waveOutOpen(&g_wo,WAVE_MAPPER,&wfx,0,0,0)!=MMSYSERR_NOERROR){ g_sndOn=0; return; }
  g_sndOn=1;
  for(i=0;i<SND_BUFS;i++){
    g_whdr[i].lpData=(LPSTR)g_wbuf[i]; g_whdr[i].dwBufferLength=SND_SAMP*2;
    waveOutPrepareHeader(g_wo,&g_whdr[i],sizeof(WAVEHDR));
    snd_fill(g_wbuf[i]);
    waveOutWrite(g_wo,&g_whdr[i],sizeof(WAVEHDR));
  }
}
static void snd_update(void){
  int i;
  if(!g_sndOn) return;
  for(i=0;i<SND_BUFS;i++) if(g_whdr[i].dwFlags&WHDR_DONE){
    g_whdr[i].dwFlags&=~WHDR_DONE;
    snd_fill(g_wbuf[i]);
    waveOutWrite(g_wo,&g_whdr[i],sizeof(WAVEHDR));
  }
}

/* 상수 (DESIGN §21) */
#define MAXSPEED 220.0f
#define ACCEL 2000.0f
#define FRICTION 2400.0f
#define DASH_SPEED 700.0f
#define DASH_TIME 0.14f
#define DASH_CD 0.8f
#define DASH_IGRACE 0.05f
#define GW 9
#define GH 9
#define CELLT 22
#define MAPW (GW*CELLT)
#define MAPH (GH*CELLT)
#define TILEF 48.0f
#define SGX 4
#define SGY 4
#define BUL_SPEED 700.0f
#define BUL_R 5.0f
#define BUL_DMG 10.0f
#define BUL_LIFE 1.2f
#define FIRE_INT 0.20f
#define EBUL_SPEED 280.0f
#define MAXBUL 256
#define MAXEBUL 256
#define MAXENE 128
#define MAXPART 768
#define MAXPICK 256
#define MAXRING 32
#define MAXAFTER 32
#define SHAKE_MAX 20.0f
#define TRAUMA_DECAY 1.5f

/* 게임 상태 (§17) */
#define ST_TITLE 0
#define ST_PLAY 1
#define ST_PAUSE 2
#define ST_OVER 3
#define ST_UPG 4
static int g_state=ST_TITLE;

/* ---- 모듈 시스템 (§7): 14 커먼 + 6 레어, 스택 ---- */
#define M_POWER 0
#define M_RAPID 1
#define M_AGI 2
#define M_MULTI 3
#define M_PIERCE 4
#define M_HOMING 5
#define M_LEECH 6
#define M_EXPLODE 7
#define M_BOUNCE 8
#define M_COOL 9
#define M_THORNS 10
#define M_CRIT 11
#define M_SWIFT 12
#define M_MAGNET 13
#define M_VENGEANCE 14
#define M_KINETIC 15
#define M_FRENZY 16
#define M_SIPHON 17
#define M_SPLIT 18
#define M_TIMEWARP 19
#define M_OVERLOAD 20
#define M_REGEN 21
#define M_AEGIS 22
#define M_HEART 23
#define M_GLASSCANNON 24
#define M_CHAINLIGHT 25
#define MODN 26
#define MOD_COMMON 18
static int g_mod[MODN];
static int g_upgSel[3], g_upgRare, g_shotCount, g_leechKills, g_shieldUp, g_siphonK;
static float g_regenT, g_shieldT, g_thornT, g_slowT, g_slowAcc, g_frenzy;
static const char* g_modName[MODN]={"POWER","RAPID","AGILITY","MULTISHOT","PIERCE","HOMING","LEECH","EXPLOSIVE","RICOCHET","COOLANT","THORNS","CRITICAL","SWIFT SHOT","MAGNET","VENGEANCE","KINETIC","FRENZY","SIPHON","SPLITSHOT","TIMEWARP","OVERLOAD","REGEN","AEGIS","EXTRA CORE","GLASS CANNON","ARC CHAIN"};
static const char* g_modDesc[MODN]={"DMG +25","FIRE RATE +20","MOVE SPD +12","SHOT +1","PIERCE +1","SEEK TARGETS","KILLS HEAL HP","HITS EXPLODE","WALL BOUNCE +2","DASH CD -25","CONTACT DMG","CRIT +15","BULLET SPD +30","PICKUP RANGE UP","LOW HP = DMG","SPD = DMG","HOLD RAMPS ROF","KILLS DROP BITS","HITS SPLIT 2","HURT SLOWMO","10TH SHOT X4","HP OVER TIME","BLOCK SHIELD","MAX HP +2 HEAL","+60% DMG RISK","HIT ARCS DMG"};
static const unsigned char g_modW[MOD_COMMON]={10,10,10,7,8,6,7,6,6,7,7,8,8,7,7,8,7,7};
static int mod_capped(int m){
  if(m==M_MULTI) return g_mod[m]>=6;
  if(m==M_CRIT) return g_mod[m]>=7;
  if(m==M_AGI) return g_mod[m]>=6;
  if(m==M_COOL) return g_mod[m]>=5;
  return 0;
}
static float dash_cd_max(void){ float c=DASH_CD; int i; for(i=0;i<g_mod[M_COOL];i++) c*=0.75f; if(c<0.2f)c=0.2f; return c; }
static void draw3(int rare){ /* 시너지 가중 추첨 (§7), 슬롯 간 중복 금지 */
  int k,i;
  g_upgSel[0]=g_upgSel[1]=g_upgSel[2]=-1;
  for(k=0;k<3;k++){
    int w[MOD_COMMON], total=0, n=rare?8:MOD_COMMON;
    for(i=0;i<n;i++){
      int mi=rare?MOD_COMMON+i:i, dup=0, q;
      w[i]=0;
      for(q=0;q<k;q++) if(g_upgSel[q]==mi) dup=1;
      if(dup) continue;
      if(!rare){
        if(mod_capped(i)) continue;
        int ww=g_modW[i];
        if(g_mod[M_MULTI]&&(i==M_PIERCE||i==M_EXPLODE||i==M_CRIT)) ww*=2;
        if(g_mod[M_HOMING]&&i==M_RAPID) ww*=2;
        if(g_mod[M_EXPLODE]&&i==M_MULTI) ww*=2;
        if(g_mod[M_THORNS]&&(i==M_COOL||i==M_AGI)) ww*=2;
        if((g_mod[M_AGI]||g_mod[M_SWIFT])&&i==M_KINETIC) ww*=2;
        if(g_mod[M_KINETIC]&&(i==M_AGI||i==M_SWIFT)) ww*=2;
        if(g_mod[M_RAPID]&&i==M_FRENZY) ww*=2;
        if(g_mod[M_MAGNET]&&i==M_SIPHON) ww*=2;
        if(g_mod[M_SIPHON]&&i==M_MAGNET) ww*=2;
        w[i]=ww;
      } else { w[i]=10; if(mi==M_GLASSCANNON&&g_mod[M_GLASSCANNON]>=2) w[i]=0; } /* GC 캡2: 3스택 무의미 추첨 방지 */
      total+=w[i];
    }
    if(total<=0) continue;
    int r2=(int)(xrnd()%(unsigned)total);
    for(i=0;i<n;i++){ if(!w[i]) continue; r2-=w[i]; if(r2<0){ g_upgSel[k]=rare?MOD_COMMON+i:i; break; } }
  }
}

/* 팰릿 5티어 순환 (§15): 냉각/연산/메모리/전력/커널 */
static const float g_palBg[5][3]={{0.031f,0.039f,0.063f},{0.055f,0.031f,0.063f},{0.063f,0.047f,0.024f},{0.024f,0.063f,0.039f},{0.039f,0.031f,0.071f}};
static const float g_palAc[5][3]={{0.0f,0.90f,1.0f},{1.0f,0.24f,0.78f},{1.0f,0.67f,0.16f},{0.24f,1.0f,0.55f},{0.67f,0.35f,1.0f}};
static float g_optShake=1.0f, g_optFlash=1.0f, g_optCRT=1.0f; /* 옵션: 광과민 대응 (§18) — CRT/포스트FX 마스터 */

static unsigned char g_tiles[MAPH][MAPW];
typedef struct { unsigned char placed, type; int rx, ry, rw, rh, dist; } Cell;
static Cell g_grid[GH][GW];
static unsigned char g_conn[GH][GW], g_cleared[GH][GW], g_seen[GH][GW];
static int g_downGX=SGX, g_downGY=SGY, g_depth=1, g_locked, g_lockGX, g_lockGY;
static int pal_tier(void){ return (g_depth-1)%5; }

typedef struct { vec2 pos, vel; float radius; } Player;
static Player g_player;
static vec2 g_cam;
static float g_pHP=6.0f, g_pMaxHP=6.0f, g_pIfr, g_fireCd;
static float g_dashT, g_dashCd, g_afterTimer;
static vec2 g_dashDir;
static float g_time; /* sim 누적 시간(비주얼 펄스용) */
/* 스킬 (docs/06 §2): RMB 블링크 + Q EMP */
#define BLINK_CD 3.5f
#define BLINK_RANGE 180.0f
#define EMP_CD 6.0f
#define EMP_R 150.0f
#define EMP_DMG 25.0f
static float g_blinkCd, g_empCd;
static int g_rmbPressed;
static int g_wantDash, g_wantEmp; /* 입력 래치: 히트스톱/슬로모/고주사율 프레임에서 유실 방지 */

/* 점수/BITS (§13) */
static int g_kills, g_bits, g_bossKills, g_bestScore, g_bestLayer;
static int score_now(void){ return g_depth*1000 + g_kills*25 + g_bits*5 + g_bossKills*500; }

/* juice (§14) */
static float g_trauma, g_hitstop, g_hurtFx, g_clearFx, g_fringeFx; /* g_fringeFx: 색수차 트리거(시각 전용) */
static vec2 g_kick;
static void add_trauma(float t){ g_trauma+=t; if(g_trauma>1.0f)g_trauma=1.0f; }

typedef struct { vec2 pos, vel; float life, dmg, r; unsigned char active, pierce, bounce, mini, lastHit; } Bullet;
/* 적 타입: 0헌터 1터릿 2리코셰 3포크 4샤드. affix 비트: 1보호막 2신속 4폭발사망 8자가분열 16자성 (§8.5) */
typedef struct { vec2 pos, vel, kvel; float hp, maxhp, t, t2, flash, spawn, at, at2; unsigned char active, type, affix, eshield; } Enemy;
typedef struct { vec2 pos; float t, t0; unsigned char active; } Boom; /* 폭발사망 지연 폭탄 (t0=초기 수명, 텔레그래프용) */
typedef struct { vec2 pos, vel; float life, ilife, r, cr, cg, cb; unsigned char active; } Part;
typedef struct { vec2 pos, vel; float t; unsigned char active, homing, kind; } Pickup; /* kind: 0=BITS 1=모듈오브 */
typedef struct { vec2 pos; float r, vr, life, ilife, cr, cg, cb; unsigned char active; } Ring;
typedef struct { vec2 pos; float life; unsigned char active; } After;
static Bullet g_bul[MAXBUL], g_ebul[MAXEBUL];
static Enemy g_ene[MAXENE];
static Part g_part[MAXPART];
static Pickup g_pick[MAXPICK];
static Ring g_ring[MAXRING];
static After g_after[MAXAFTER];
#define MAXBOOM 24
static Boom g_boom[MAXBOOM];

/* ---- 보스 (§9): k=depth/3, 홀수 CORE·짝수 WARDEN ---- */
typedef struct {
  int active, type, phase, dashState; /* type 0=CORE 1=WARDEN. dashState 0대기 1조준 2돌진 3정지 */
  float hp, maxhp, flash;
  vec2 pos, dashDir, ldir, wdir;
  float t1, t2, t3, segAng, shrink, trailT, stateT;
} Boss;
static Boss g_boss;
static int g_bossDead; /* 현재 레이어 보스 처치 여부 */
typedef struct { vec2 pos; float life; unsigned char active; } TrailP;
#define MAXTRAIL 96
static TrailP g_trail[MAXTRAIL];
static int boss_layer(void){ return (g_depth%3)==0; }
static float boss_radius(void){ return g_boss.type==0?50.0f:(g_boss.type==2?45.0f:60.0f); }
/* 빔 충돌: 원점(bx,by) 방향(ux,uy 단위) 반두께 halfw 길이 maxlen vs 원(px,py,pr) */
static int beam_hit(float bx,float by,float ux,float uy,float halfw,float px,float py,float pr,float maxlen){
  float rx=px-bx, ry=py-by, along=rx*ux+ry*uy;
  if(along<0.0f||along>maxlen) return 0;
  float d=rx*uy-ry*ux; if(d<0.0f)d=-d;
  return d<halfw+pr;
}
static void vrot(vec2*v,float a){ float ca=f_cos(a), sa=f_sin(a), x=v->x; v->x=x*ca-v->y*sa; v->y=x*sa+v->y*ca; }

static const PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1,
  PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,32,
  0,0,0,0,0,0,0,0,0,0,0,0,0,24,8,0,PFD_MAIN_PLANE,0,0,0,0 };

/* 타일: 0=벽 1=바닥 2=복도바닥(장애물 금지) 3=기둥 4=크레이트(파괴 가능) — docs/06 §3 */
#define T_SOLID(t) ((t)==0||(t)==3||(t)==4)
#define T_WALK(t) ((t)==1||(t)==2)
static int is_wall_w(float wx, float wy){
  int tx=(int)(wx/TILEF), ty=(int)(wy/TILEF);
  if(tx<0||ty<0||tx>=MAPW||ty>=MAPH) return 1;
  return T_SOLID(g_tiles[ty][tx]);
}
static float ene_radius(int t){
  if(t==0) return 13.0f; if(t==1) return 14.0f; if(t==2) return 11.0f;
  if(t==3) return 18.0f; if(t==5) return 12.0f; if(t==6) return 13.0f; return 8.0f;
}
static float ene_speed(int t){
  float s=t==0?130.0f:(t==1?70.0f:(t==2?230.0f:(t==3?60.0f:(t==5?110.0f:(t==6?95.0f:160.0f)))));
  float sc=1.0f+0.04f*(float)(g_depth-1); if(sc>1.6f)sc=1.6f; return s*sc;
}

/* ---- 3x5 비트맵 폰트 (§15 — 파일 없음, 코드 인코딩) ---- */
#define GF(a,b,c,d,e) (unsigned short)(((a)<<12)|((b)<<9)|((c)<<6)|((d)<<3)|(e))
static const unsigned short g_font[40]={
  GF(7,5,5,5,7),GF(2,6,2,2,7),GF(7,1,7,4,7),GF(7,1,7,1,7),GF(5,5,7,1,1), /* 0-4 */
  GF(7,4,7,1,7),GF(7,4,7,5,7),GF(7,1,1,2,2),GF(7,5,7,5,7),GF(7,5,7,1,7), /* 5-9 */
  GF(2,5,7,5,5),GF(6,5,6,5,6),GF(7,4,4,4,7),GF(6,5,5,5,6),GF(7,4,7,4,7), /* A-E */
  GF(7,4,7,4,4),GF(7,4,5,5,7),GF(5,5,7,5,5),GF(7,2,2,2,7),GF(1,1,1,5,7), /* F-J */
  GF(5,5,6,5,5),GF(4,4,4,4,7),GF(5,7,7,5,5),GF(6,5,5,5,5),GF(7,5,5,5,7), /* K-O */
  GF(7,5,7,4,4),GF(7,5,5,7,1),GF(7,5,6,5,5),GF(7,4,7,1,7),GF(7,2,2,2,2), /* P-T */
  GF(5,5,5,5,7),GF(5,5,5,5,2),GF(5,5,7,7,5),GF(5,5,2,5,5),GF(5,5,2,2,2), /* U-Y */
  GF(7,1,2,4,7),GF(0,0,7,0,0),GF(0,0,0,0,2),GF(0,2,0,2,0),GF(1,1,2,4,4)  /* Z - . : / */
};
static int glyph_idx(char c){
  if(c>='0'&&c<='9') return c-'0';
  if(c>='A'&&c<='Z') return 10+c-'A';
  if(c=='-') return 36; if(c=='.') return 37; if(c==':') return 38; if(c=='/') return 39;
  return -1;
}
static void draw_text(float x,float y,float s,const char *t,float r,float g,float b,float a){
  glColor4f(r,g,b,a); glBegin(GL_QUADS);
  for(;*t;t++){
    int gi=glyph_idx(*t);
    if(gi>=0){ unsigned short bits=g_font[gi]; int row,col;
      for(row=0;row<5;row++) for(col=0;col<3;col++)
        if((bits>>((4-row)*3+(2-col)))&1){ float px=x+col*s, py=y+row*s;
          glVertex2f(px,py); glVertex2f(px+s,py); glVertex2f(px+s,py+s); glVertex2f(px,py+s); }
    }
    x+=4.0f*s;
  }
  glEnd();
}
static float text_w(const char *t,float s){ int n=0; for(;*t;t++)n++; return n>0?((float)n*4.0f-1.0f)*s:0.0f; }
static void fmt_int(char *buf,int v){ /* 자작 정수→글리프 (printf 금지, rules/20) */
  char tmp[12]; int n=0,i;
  if(v<0)v=0;
  do{ tmp[n++]='0'+(v%10); v/=10; }while(v&&n<11);
  for(i=0;i<n;i++) buf[i]=tmp[n-1-i];
  buf[n]=0;
}
static void draw_int(float x,float y,float s,int v,float r,float g,float b,float a){
  char buf[12]; fmt_int(buf,v); draw_text(x,y,s,buf,r,g,b,a);
}
static void center_text(int w,float y,float s,const char *t,float r,float g,float b,float a){
  draw_text(((float)w-text_w(t,s))*0.5f,y,s,t,r,g,b,a);
}

/* ---- 파티클/링/잔상 ---- */
static void spawn_part(float x,float y,float vx,float vy,float life,float r,float cr,float cg,float cb){
  int i; for(i=0;i<MAXPART;i++) if(!g_part[i].active){ Part*p=&g_part[i];
    p->active=1; p->pos.x=x; p->pos.y=y; p->vel.x=vx; p->vel.y=vy; p->life=life; p->ilife=life; p->r=r; p->cr=cr; p->cg=cg; p->cb=cb; return; }
}
static void burst(float x,float y,int n,float spd,float cr,float cg,float cb){
  /* 순수 코스메틱 산란 → g_rngFx 사용(sim 시드 비오염, det 규칙) */
  int i; for(i=0;i<n;i++){ float dx=fxsym(), dy=fxsym(), l=f_sqrt(dx*dx+dy*dy); if(l<0.01f)l=1.0f;
    float s=spd*(0.4f+(fxsym()*0.5f+0.5f)*0.6f);
    spawn_part(x,y,dx/l*s,dy/l*s,0.25f+(fxsym()*0.5f+0.5f)*0.35f,2.0f+(fxsym()*0.5f+0.5f)*3.0f,cr,cg,cb); }
}
static void spawn_ring(float x,float y,float r0,float vr,float life,float cr,float cg,float cb){
  int i; for(i=0;i<MAXRING;i++) if(!g_ring[i].active){ Ring*q=&g_ring[i];
    q->active=1; q->pos.x=x; q->pos.y=y; q->r=r0; q->vr=vr; q->life=life; q->ilife=life; q->cr=cr; q->cg=cg; q->cb=cb; return; }
}
static void spawn_after(float x,float y){
  int i; for(i=0;i<MAXAFTER;i++) if(!g_after[i].active){ g_after[i].active=1; g_after[i].pos.x=x; g_after[i].pos.y=y; g_after[i].life=0.22f; return; }
}
static void spawn_pickup(float x,float y,int kind){
  int i; for(i=0;i<MAXPICK;i++) if(!g_pick[i].active){ Pickup*p=&g_pick[i];
    p->active=1; p->homing=0; p->kind=(unsigned char)kind; p->pos.x=x; p->pos.y=y; p->t=rnd01()*6.28f;
    p->vel.x=rndsym()*90.0f; p->vel.y=rndsym()*90.0f; return; }
}

/* ---- 던전 (§10) ---- */
static int popcnt(unsigned char v){ int c=0; while(v){ c+=v&1; v>>=1; } return c; }
static void set_conn(int ax,int ay,int bx,int by){
  if(bx==ax&&by==ay-1){ g_conn[ay][ax]|=1; g_conn[by][bx]|=4; }
  else if(bx==ax+1&&by==ay){ g_conn[ay][ax]|=2; g_conn[by][bx]|=8; }
  else if(bx==ax&&by==ay+1){ g_conn[ay][ax]|=4; g_conn[by][bx]|=1; }
  else if(bx==ax-1&&by==ay){ g_conn[ay][ax]|=8; g_conn[by][bx]|=2; }
}
static void put_floor(int x,int y){ if(x>=0&&y>=0&&x<MAPW&&y<MAPH) g_tiles[y][x]=1; }
static void put_corr(int x,int y){ if(x>=0&&y>=0&&x<MAPW&&y<MAPH) g_tiles[y][x]=2; }
static void carve_corr(int ax,int ay,int bx,int by){
  int x,y,x0=ax<bx?ax:bx,x1=ax<bx?bx:ax,y0=ay<by?ay:by,y1=ay<by?by:ay;
  for(x=x0;x<=x1;x++){ put_corr(x,ay); put_corr(x,ay+1); }
  for(y=y0;y<=y1;y++){ put_corr(bx,y); put_corr(bx+1,y); }
}
/* 장애물 배치 가능 검사: 풋프린트=순수 바닥(1)·중심 3x3 밖, 주변 8방에 다른 장애물 없음 */
static int obst_fit(int ox,int oy,int sz,int ccx,int ccy){
  int x,y;
  for(y=oy-1;y<=oy+sz;y++) for(x=ox-1;x<=ox+sz;x++){
    if(x<0||y<0||x>=MAPW||y>=MAPH) return 0;
    unsigned char t=g_tiles[y][x];
    if(x>=ox&&x<ox+sz&&y>=oy&&y<oy+sz){
      if(t!=1) return 0;
      if(x>=ccx-1&&x<=ccx+1&&y>=ccy-1&&y<=ccy+1) return 0;
    } else if(t==3||t==4) return 0;
  }
  return 1;
}
static int try_break_crate(float wx,float wy){
  int tx=(int)(wx/TILEF), ty=(int)(wy/TILEF);
  if(tx<0||ty<0||tx>=MAPW||ty>=MAPH) return 0;
  if(g_tiles[ty][tx]!=4) return 0;
  g_tiles[ty][tx]=1;
  burst((tx+0.5f)*TILEF,(ty+0.5f)*TILEF,10,210.0f,1.0f,0.7f,0.25f);
  if((xrnd()%100)<20) spawn_pickup((tx+0.5f)*TILEF,(ty+0.5f)*TILEF,0);
  snd_play(SFX_HIT);
  return 1;
}
static void generate(void){
  int gx,gy,x,y,k;
  memset(g_tiles,0,sizeof(g_tiles)); memset(g_grid,0,sizeof(g_grid)); memset(g_conn,0,sizeof(g_conn));
  memset(g_cleared,0,sizeof(g_cleared)); memset(g_seen,0,sizeof(g_seen));
  memset(g_ene,0,sizeof(g_ene)); memset(g_bul,0,sizeof(g_bul)); memset(g_ebul,0,sizeof(g_ebul));
  memset(g_part,0,sizeof(g_part)); memset(g_pick,0,sizeof(g_pick)); memset(g_ring,0,sizeof(g_ring)); memset(g_after,0,sizeof(g_after)); memset(g_boom,0,sizeof(g_boom)); memset(g_trail,0,sizeof(g_trail));
  g_boss.active=0; g_bossDead=0;
  g_locked=0; g_fireCd=0.0f;

  int N=4+g_depth; if(N>12)N=12;
  int px[81], py[81], np=1, count=1, guard=0;
  g_grid[SGY][SGX].placed=1; px[0]=SGX; py[0]=SGY;
  while(count<N && guard<200000){
    guard++;
    int i=(int)(xrnd()%(unsigned)np), cgx=px[i], cgy=py[i], d=(int)(xrnd()&3), nx=cgx, ny=cgy;
    if(d==0)ny--; else if(d==1)nx++; else if(d==2)ny++; else nx--;
    if(nx<0||ny<0||nx>=GW||ny>=GH) continue;
    if(!g_grid[ny][nx].placed){ g_grid[ny][nx].placed=1; px[np]=nx; py[np]=ny; np++; count++; set_conn(cgx,cgy,nx,ny); }
    else if((xrnd()%100)<15) set_conn(cgx,cgy,nx,ny);
  }
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++){
    if(!g_grid[gy][gx].placed) continue;
    int rw=11+(int)(xrnd()%7), rh=9+(int)(xrnd()%5), rx=gx*CELLT+(CELLT-rw)/2, ry=gy*CELLT+(CELLT-rh)/2;
    g_grid[gy][gx].rx=rx; g_grid[gy][gx].ry=ry; g_grid[gy][gx].rw=rw; g_grid[gy][gx].rh=rh; g_grid[gy][gx].type=1;
    for(y=ry;y<ry+rh;y++) for(x=rx;x<rx+rw;x++) put_floor(x,y);
  }
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++){
    if(!g_grid[gy][gx].placed) continue;
    int acx=g_grid[gy][gx].rx+g_grid[gy][gx].rw/2, acy=g_grid[gy][gx].ry+g_grid[gy][gx].rh/2;
    if((g_conn[gy][gx]&2)&&gx+1<GW&&g_grid[gy][gx+1].placed) carve_corr(acx,acy,g_grid[gy][gx+1].rx+g_grid[gy][gx+1].rw/2,g_grid[gy][gx+1].ry+g_grid[gy][gx+1].rh/2);
    if((g_conn[gy][gx]&4)&&gy+1<GH&&g_grid[gy+1][gx].placed) carve_corr(acx,acy,g_grid[gy+1][gx].rx+g_grid[gy+1][gx].rw/2,g_grid[gy+1][gx].ry+g_grid[gy+1][gx].rh/2);
  }
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) g_grid[gy][gx].dist=-1;
  int qx[81], qy[81], qh=0, qt=0;
  g_grid[SGY][SGX].dist=0; qx[qt]=SGX; qy[qt]=SGY; qt++;
  while(qh<qt){
    int cgx=qx[qh], cgy=qy[qh]; qh++;
    unsigned char c=g_conn[cgy][cgx];
    int nb[4][2]={{cgx,cgy-1},{cgx+1,cgy},{cgx,cgy+1},{cgx-1,cgy}}, bit[4]={1,2,4,8};
    for(k=0;k<4;k++){ if(!(c&bit[k])) continue; int nx=nb[k][0], ny=nb[k][1];
      if(nx<0||ny<0||nx>=GW||ny>=GH||!g_grid[ny][nx].placed||g_grid[ny][nx].dist>=0) continue;
      g_grid[ny][nx].dist=g_grid[cgy][cgx].dist+1; qx[qt]=nx; qy[qt]=ny; qt++; }
  }
  int bestd=-1, dgx=SGX, dgy=SGY;
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) if(g_grid[gy][gx].placed&&g_grid[gy][gx].dist>bestd){ bestd=g_grid[gy][gx].dist; dgx=gx; dgy=gy; }
  g_grid[dgy][dgx].type=3; g_grid[SGY][SGX].type=0;
  for(gy=0;gy<GH;gy++){ for(gx=0;gx<GW;gx++){ if(!g_grid[gy][gx].placed) continue;
    if((gx==SGX&&gy==SGY)||(gx==dgx&&gy==dgy)) continue;
    if(popcnt(g_conn[gy][gx])==1){ g_grid[gy][gx].type=2; gy=GH; break; } } }
  g_downGX=dgx; g_downGY=dgy;
  g_grid[SGY][SGX].type=0;
  g_cleared[SGY][SGX]=1;  /* 시작방은 안전(클리어 취급) */

  /* 장애물 배치 (EXT-A, docs/06 §3): 기둥 1~3 + 크레이트 2~5, 시작방 제외 */
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++){
    Cell*c=&g_grid[gy][gx];
    if(!c->placed) continue;
    if(gx==SGX&&gy==SGY) continue;
    int ccx=c->rx+c->rw/2, ccy=c->ry+c->rh/2;
    int wantP=1+(int)(xrnd()%3), wantC=2+(int)(xrnd()%4), got, tr;
    for(got=0,tr=0;got<wantP&&tr<40;tr++){
      int sz=1+(int)(xrnd()&1);
      if(c->rw<sz+5||c->rh<sz+5) break;
      int ox=c->rx+2+(int)(xrnd()%(unsigned)(c->rw-3-sz));
      int oy=c->ry+2+(int)(xrnd()%(unsigned)(c->rh-3-sz));
      if(obst_fit(ox,oy,sz,ccx,ccy)){ int a,b2;
        for(a=0;a<sz;a++) for(b2=0;b2<sz;b2++) g_tiles[oy+b2][ox+a]=3;
        got++; }
    }
    for(got=0,tr=0;got<wantC&&tr<40;tr++){
      int ox=c->rx+2+(int)(xrnd()%(unsigned)(c->rw-4));
      int oy=c->ry+2+(int)(xrnd()%(unsigned)(c->rh-4));
      if(obst_fit(ox,oy,1,ccx,ccy)){ g_tiles[oy][ox]=4; got++; }
    }
  }
  g_player.pos.x=(g_grid[SGY][SGX].rx+g_grid[SGY][SGX].rw*0.5f)*TILEF;
  g_player.pos.y=(g_grid[SGY][SGX].ry+g_grid[SGY][SGX].rh*0.5f)*TILEF;
  g_player.vel.x=0; g_player.vel.y=0; g_cam=g_player.pos;
}
static void descend(void){ g_depth++; g_rng=g_master^((unsigned)g_depth*2654435761u); if(!g_rng)g_rng=1; (void)xrnd(); (void)xrnd(); generate(); }
static void new_run(void){
  LARGE_INTEGER li; QueryPerformanceCounter(&li); g_master=(unsigned int)li.QuadPart|1u;
  g_depth=1; g_rng=g_master^2654435761u; if(!g_rng)g_rng=1; (void)xrnd();
  g_pHP=g_pMaxHP=6.0f; g_pIfr=0; g_kills=0; g_bits=0; g_bossKills=0;
  g_dashT=0; g_dashCd=0; g_trauma=0; g_hitstop=0; g_hurtFx=0; g_clearFx=0; g_fringeFx=0; g_time=0;
  g_blinkCd=0; g_empCd=0; g_rmbPressed=0; g_wantDash=0; g_wantEmp=0;
  memset(g_mod,0,sizeof(g_mod));
  g_shotCount=0; g_leechKills=0; g_shieldUp=0;
  g_regenT=0; g_shieldT=0; g_thornT=0; g_slowT=0; g_slowAcc=0; g_frenzy=0; g_siphonK=0;
  generate();
  g_state=ST_PLAY;
}

/* ---- 적 스폰 (0.4s 텔레그래프, §11; 깊이 게이팅+엘리트 §12·§8.5) ---- */
static int spawn_enemy(int type,float x,float y,int allowElite){
  int i; for(i=0;i<MAXENE;i++) if(!g_ene[i].active){ Enemy*e=&g_ene[i];
    e->active=1; e->type=(unsigned char)type; e->pos.x=x; e->pos.y=y; e->vel.x=0; e->vel.y=0; e->kvel.x=0; e->kvel.y=0;
    e->t=0; e->t2=0; e->flash=0; e->spawn=0.4f; e->at=0; e->at2=0; e->affix=0; e->eshield=0;
    float hpsc=1.0f+0.18f*(float)(g_depth-1);
    if(type==0) e->hp=20.0f*hpsc;
    else if(type==1) e->hp=16.0f*hpsc;
    else if(type==2){ e->hp=12.0f*hpsc; float dx=rndsym(), dy=rndsym(), l=f_sqrt(dx*dx+dy*dy); if(l<0.01f)l=1.0f; float sp=ene_speed(2); e->vel.x=dx/l*sp; e->vel.y=dy/l*sp; }
    else if(type==3) e->hp=30.0f*hpsc;
    else if(type==5) e->hp=18.0f*hpsc; /* LANCER */
    else if(type==6) e->hp=22.0f*hpsc; /* WEAVER */
    else e->hp=8.0f*hpsc;
    if(allowElite&&type!=4){ /* 엘리트 어픽스 (§8.5): min(0.25, 0.03*(d-1)) */
      float ep=0.03f*(float)(g_depth-1); if(ep>0.25f)ep=0.25f;
      if(rnd01()<ep){
        int na=(g_depth>=7&&(xrnd()&1))?2:1;
        int b1=(int)(xrnd()%5); e->affix|=(unsigned char)(1<<b1);
        if(na==2){ int b2; do{ b2=(int)(xrnd()%5); }while(b2==b1); e->affix|=(unsigned char)(1<<b2); } /* §8.5 중복 불가 */
        if(e->affix&1) e->eshield=1;
        e->hp*=1.15f;
      }
    }
    e->maxhp=e->hp;
    return i; }
  return -1;
}
static int pick_enemy_type(void){ /* 해금: L1 헌터/L2 터릿/L3 리코셰/L4 포크/L5 랜서/L6 위버. 초반 헌터 가중 */
  int types[6], n=1; types[0]=0;
  if(g_depth>=2) types[n++]=1;
  if(g_depth>=3) types[n++]=2;
  if(g_depth>=4) types[n++]=3;
  if(g_depth>=5) types[n++]=5;
  if(g_depth>=6) types[n++]=6;
  int w[6], i, tot=0;
  for(i=0;i<n;i++){ w[i]=(types[i]==0)?(g_depth>=6?8:10):(g_depth>=6?8:6); tot+=w[i]; }
  int r=(int)(xrnd()%(unsigned)tot);
  for(i=0;i<n;i++){ r-=w[i]; if(r<0) return types[i]; }
  return 0;
}
static void spawn_wave(int gx,int gy){
  Cell*c=&g_grid[gy][gx];
  int cnt=4+g_depth; if(cnt>12)cnt=12;
  int placed=0, tries=0;
  while(placed<cnt && tries<500){ tries++;
    int tx=c->rx+1+(int)(xrnd()%(unsigned)(c->rw-2)), ty=c->ry+1+(int)(xrnd()%(unsigned)(c->rh-2));
    float wx=(tx+0.5f)*TILEF, wy=(ty+0.5f)*TILEF, dx=wx-g_player.pos.x, dy=wy-g_player.pos.y;
    if(!T_WALK(g_tiles[ty][tx])) continue;
    if(dx*dx+dy*dy<150.0f*150.0f) continue;
    spawn_enemy(pick_enemy_type(),wx,wy,1);
    placed++;
  }
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l){
  switch(m){
    case WM_CLOSE: case WM_DESTROY: g_quit=1; PostQuitMessage(0); return 0;
    case WM_KEYDOWN: if(w<256){ if(!(l&(1<<30))) g_kpress[w]=1; g_keys[w]=1; } return 0;
    case WM_KEYUP: if(w<256) g_keys[w]=0; return 0;
    case WM_MOUSEMOVE: g_mouseX=(short)LOWORD(l); g_mouseY=(short)HIWORD(l); return 0;
    case WM_LBUTTONDOWN: g_mouseDown=1; g_mousePressed=1; return 0;
    case WM_LBUTTONUP: g_mouseDown=0; return 0;
    case WM_RBUTTONDOWN: g_rmbPressed=1; return 0;
  }
  return DefWindowProcA(h,m,w,l);
}

static void apply_mod(int m){
  if(m<0||m>=MODN) return;
  g_mod[m]++;
  if(m==M_HEART){ g_pMaxHP+=2.0f; g_pHP=g_pMaxHP; }
  snd_play(SFX_PICKUP);
  g_state=ST_PLAY;
}
static void hurt_player(float dmg){
  if(g_pIfr>0.0f || g_dashT>0.0f) return;
  if(g_shieldUp){ /* 수호막 (레어): 1회 피격 무효 */
    g_shieldUp=0; g_shieldT=0.0f; g_pIfr=0.5f;
    spawn_ring(g_player.pos.x,g_player.pos.y,16.0f,400.0f,0.3f,0.4f,0.9f,1.0f);
    add_trauma(0.2f); snd_play(SFX_HIT);
    return;
  }
  g_pHP-=dmg+(float)(g_mod[M_GLASSCANNON]>0); g_pIfr=1.0f; /* 글래스캐논: 피격당 +1 추가뎀 (무한생존 차단 §7) */
  add_trauma(0.5f); g_hitstop=0.08f; g_hurtFx=1.0f;
  if(g_mod[M_TIMEWARP]){ g_slowT=0.3f; g_slowAcc=0.0f; } /* 시간왜곡 (레어) */
  snd_play(SFX_HURT);
  burst(g_player.pos.x,g_player.pos.y,10,200.0f,1.0f,0.25f,0.35f);
  if(g_pHP<=0.0f){ /* 사망 → 게임오버 (§17) */
    int sc=score_now();
    if(sc>g_bestScore){ g_bestScore=sc; }
    if(g_depth>g_bestLayer) g_bestLayer=g_depth;
    g_hitstop=0.2f; add_trauma(0.8f);
    burst(g_player.pos.x,g_player.pos.y,32,320.0f,1.0f,0.9f,0.9f);
    spawn_ring(g_player.pos.x,g_player.pos.y,10.0f,420.0f,0.5f,1.0f,0.4f,0.5f);
    g_state=ST_OVER;
  }
}

static void spawn_ebul(float x,float y,float ux,float uy,float spd,float life){
  int i; for(i=0;i<MAXEBUL;i++) if(!g_ebul[i].active){ Bullet*b=&g_ebul[i];
    b->active=1; b->pos.x=x; b->pos.y=y; b->vel.x=ux*spd; b->vel.y=uy*spd; b->life=life; return; }
}
static void boss_die(void){
  int i;
  g_boss.active=0; g_bossDead=1; g_bossKills++;
  g_hitstop=0.2f; add_trauma(0.8f);
  burst(g_boss.pos.x,g_boss.pos.y,48,420.0f,1.0f,0.9f,0.9f);
  spawn_ring(g_boss.pos.x,g_boss.pos.y,20.0f,700.0f,0.5f,1.0f,1.0f,1.0f);
  spawn_ring(g_boss.pos.x,g_boss.pos.y,10.0f,420.0f,0.4f,1.0f,0.5f,0.7f);
  snd_play(SFX_BOSSDIE);
  for(i=0;i<MAXENE;i++) if(g_ene[i].active){ g_ene[i].active=0; burst(g_ene[i].pos.x,g_ene[i].pos.y,8,180.0f,1.0f,0.6f,0.3f); }
  for(i=0;i<MAXEBUL;i++) g_ebul[i].active=0;
  for(i=0;i<MAXTRAIL;i++) g_trail[i].active=0;
  g_pHP+=2.0f; if(g_pHP>g_pMaxHP)g_pHP=g_pMaxHP; /* §9.3 보상 */
  draw3(1); g_upgRare=1; g_state=ST_UPG;
}
static void boss_hit(float dmg){
  if(!g_boss.active) return;
  g_boss.hp-=dmg; g_boss.flash=0.06f;
  if(g_boss.hp<=0.0f) boss_die();
}

static void player_update(float dt){
  vec2 in={0.0f,0.0f};
  if(g_keys['W']||g_keys[VK_UP])    in.y-=1.0f;
  if(g_keys['S']||g_keys[VK_DOWN])  in.y+=1.0f;
  if(g_keys['A']||g_keys[VK_LEFT])  in.x-=1.0f;
  if(g_keys['D']||g_keys[VK_RIGHT]) in.x+=1.0f;
  float il=f_sqrt(in.x*in.x+in.y*in.y);
  if(il>0.0001f){ in.x/=il; in.y/=il; }

  /* 대시 (§5: 700px/s 0.14s, i-frame, 쿨 0.8s×냉각, 잔상) */
  if(g_dashCd>0.0f) g_dashCd-=dt;
  if(g_wantDash && g_dashCd<=0.0f && g_dashT<=0.0f){
    g_wantDash=0;
    vec2 dd=in;
    if(il<=0.0001f){ /* 입력 없으면 마우스 방향 */
      float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
      dd.x=mwx-g_player.pos.x; dd.y=mwy-g_player.pos.y;
      float dl=f_sqrt(dd.x*dd.x+dd.y*dd.y); if(dl<0.001f){dd.x=1.0f;dd.y=0;} else {dd.x/=dl; dd.y/=dl;}
    }
    g_dashT=DASH_TIME; g_dashCd=dash_cd_max(); g_dashDir=dd; g_afterTimer=0;
    spawn_ring(g_player.pos.x,g_player.pos.y,6.0f,300.0f,0.25f,0.4f,0.95f,1.0f);
    snd_play(SFX_DASH);
  }

  if(g_dashT>0.0f){ /* 대시 중: 고정 속도 + 잔상 */
    g_dashT-=dt;
    g_player.vel.x=g_dashDir.x*DASH_SPEED; g_player.vel.y=g_dashDir.y*DASH_SPEED;
    g_afterTimer-=dt;
    if(g_afterTimer<=0.0f){ g_afterTimer=0.03f; spawn_after(g_player.pos.x,g_player.pos.y); }
  } else {
    if(il>0.0001f){ g_player.vel.x+=in.x*ACCEL*dt; g_player.vel.y+=in.y*ACCEL*dt; }
    else { float s=f_sqrt(g_player.vel.x*g_player.vel.x+g_player.vel.y*g_player.vel.y);
      if(s>0.0001f){ float ns=s-FRICTION*dt; if(ns<0.0f)ns=0.0f; g_player.vel.x=g_player.vel.x/s*ns; g_player.vel.y=g_player.vel.y/s*ns; } }
    float msp=MAXSPEED*(1.0f+0.12f*(float)g_mod[M_AGI]); if(msp>MAXSPEED*1.8f)msp=MAXSPEED*1.8f; /* 민첩 (cap 1.8x) */
    float sp=f_sqrt(g_player.vel.x*g_player.vel.x+g_player.vel.y*g_player.vel.y);
    if(sp>msp){ g_player.vel.x=g_player.vel.x/sp*msp; g_player.vel.y=g_player.vel.y/sp*msp; }
  }
  float r=g_player.radius;
  float nxp=g_player.pos.x+g_player.vel.x*dt, ex=nxp+(g_player.vel.x>0.0f?r:-r);
  if(is_wall_w(ex,g_player.pos.y-r*0.6f)||is_wall_w(ex,g_player.pos.y+r*0.6f)){
    if(g_dashT>0.0f){ /* 대시 충돌로 크레이트 파괴 (docs/06 §3.3) */
      if(try_break_crate(ex,g_player.pos.y-r*0.6f)|try_break_crate(ex,g_player.pos.y+r*0.6f)) add_trauma(0.1f); }
    if(is_wall_w(ex,g_player.pos.y-r*0.6f)||is_wall_w(ex,g_player.pos.y+r*0.6f)) g_player.vel.x=0.0f; else g_player.pos.x=nxp;
  } else g_player.pos.x=nxp;
  float nyp=g_player.pos.y+g_player.vel.y*dt, ey=nyp+(g_player.vel.y>0.0f?r:-r);
  if(is_wall_w(g_player.pos.x-r*0.6f,ey)||is_wall_w(g_player.pos.x+r*0.6f,ey)){
    if(g_dashT>0.0f){
      if(try_break_crate(g_player.pos.x-r*0.6f,ey)|try_break_crate(g_player.pos.x+r*0.6f,ey)) add_trauma(0.1f); }
    if(is_wall_w(g_player.pos.x-r*0.6f,ey)||is_wall_w(g_player.pos.x+r*0.6f,ey)) g_player.vel.y=0.0f; else g_player.pos.y=nyp;
  } else g_player.pos.y=nyp;
}

static void kill_enemy(int idx){
  Enemy*e=&g_ene[idx];
  e->active=0; g_kills++;
  g_hitstop=0.03f; add_trauma(0.12f);
  snd_play(SFX_DEATH);
  burst(e->pos.x,e->pos.y,16,240.0f,1.0f,0.5f,0.2f);
  spawn_ring(e->pos.x,e->pos.y,6.0f,360.0f,0.3f,1.0f,0.6f,0.25f);
  if(e->type==3){ /* 포크: Shard 2 분열 (재분열 없음, §8.4; 벽 내부 스폰 폴백) */
    float lx=is_wall_w(e->pos.x-10.0f,e->pos.y)?e->pos.x:e->pos.x-10.0f;
    float rx=is_wall_w(e->pos.x+10.0f,e->pos.y)?e->pos.x:e->pos.x+10.0f;
    int s1=spawn_enemy(4,lx,e->pos.y,0), s2=spawn_enemy(4,rx,e->pos.y,0);
    if(s1>=0) g_ene[s1].spawn=0.15f;
    if(s2>=0) g_ene[s2].spawn=0.15f;
  }
  if(e->affix&4){ /* 폭발사망 (Volatile): 0.5s 지연 폭탄 */
    int q; for(q=0;q<MAXBOOM;q++) if(!g_boom[q].active){ g_boom[q].active=1; g_boom[q].pos=e->pos; g_boom[q].t=g_boom[q].t0=0.5f; break; }
  }
  int nb=1+(int)(xrnd()%3), b;
  if(e->affix){ nb*=3; /* 엘리트 BITS ×3 + 깊은 레이어 8% 모듈 오브 */
    if(g_depth>=5&&(xrnd()%100)<8) spawn_pickup(e->pos.x,e->pos.y,1);
  }
  for(b=0;b<nb;b++) spawn_pickup(e->pos.x,e->pos.y,0);
  if(g_mod[M_SIPHON]){ /* 흡수: N킬마다 보너스 BITS 다발 + 광란 너지 (회복 아님, §7) */
    int th=5-(g_mod[M_SIPHON]-1); if(th<2)th=2;
    g_siphonK++;
    if(g_siphonK>=th){ g_siphonK=0; int extra=2+g_mod[M_SIPHON]; if(extra>5)extra=5; int q;
      for(q=0;q<extra;q++) spawn_pickup(e->pos.x,e->pos.y,0);
      if(g_mod[M_FRENZY]){ g_frenzy+=0.10f; float fm=0.5f*(float)g_mod[M_FRENZY]; if(fm>1.5f)fm=1.5f; if(g_frenzy>fm)g_frenzy=fm; } } /* 광란 너지는 FRENZY 보유 시만 (캡 존중) */
  }
  if(g_mod[M_LEECH]){ /* 흡혈: 12킬마다 1HP (스택당 -3, 최소 3) */
    int th=12-3*(g_mod[M_LEECH]-1); if(th<3)th=3;
    g_leechKills++;
    if(g_leechKills>=th){ g_leechKills=0;
      if(g_pHP<g_pMaxHP){ g_pHP+=1.0f; spawn_ring(g_player.pos.x,g_player.pos.y,10.0f,180.0f,0.3f,0.3f,1.0f,0.5f); } }
  }
}
static void explode_at(float x,float y,float bdmg){ /* 폭발 모듈 (§7, 자해 없음) */
  int stk=g_mod[M_EXPLODE], j; if(stk<=0) return;
  float R=60.0f+25.0f*(float)(stk-1), dm=bdmg*(0.50f+0.15f*(float)(stk-1));
  for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
    float dx=e->pos.x-x, dy=e->pos.y-y, rr=R+ene_radius(e->type);
    if(dx*dx+dy*dy<rr*rr){ float d=f_sqrt(dx*dx+dy*dy); if(d<0.01f)d=0.01f;
      e->hp-=dm; e->flash=0.07f; e->kvel.x+=dx/d*120.0f; e->kvel.y+=dy/d*120.0f;
      if(e->hp<=0.0f) kill_enemy(j); } }
  if(g_boss.active){ float bdx=g_boss.pos.x-x, bdy=g_boss.pos.y-y, rr=R+boss_radius();
    if(bdx*bdx+bdy*bdy<rr*rr) boss_hit(dm); }
  burst(x,y,12,260.0f,1.0f,0.6f,0.2f);
  spawn_ring(x,y,10.0f,(R-10.0f)*5.5f,0.18f,1.0f,0.55f,0.2f);
  add_trauma(0.25f);
}
static void arc_chain(float x,float y,int skip,float dmg){ /* 체인라이트닝 (레어): 인접 1체에 단발 아크(재귀 없음, §7) */
  int j,bj=-1; float bd2=140.0f*140.0f;
  for(j=0;j<MAXENE;j++){ if(j==skip||!g_ene[j].active||g_ene[j].spawn>0.0f) continue;
    float dx=g_ene[j].pos.x-x, dy=g_ene[j].pos.y-y, d2=dx*dx+dy*dy;
    if(d2<bd2){ bd2=d2; bj=j; } }
  if(bj<0) return;
  Enemy*e=&g_ene[bj]; e->hp-=dmg; e->flash=0.06f;
  spawn_ring(e->pos.x,e->pos.y,ene_radius(e->type)+4.0f,200.0f,0.18f,0.5f,0.9f,1.0f);
  burst(e->pos.x,e->pos.y,4,160.0f,0.5f,0.9f,1.0f);
  if(e->hp<=0.0f) kill_enemy(bj);
}

static void boss_update(float dt){
  if(!g_boss.active) return;
  Boss*B=&g_boss; int k;
  Cell*c=&g_grid[g_downGY][g_downGX];
  float br=boss_radius();
  float mnx=(c->rx+1)*TILEF+br, mxx=(c->rx+c->rw-1)*TILEF-br;
  float mny=(c->ry+1)*TILEF+br, mxy=(c->ry+c->rh-1)*TILEF-br;
  float rcx=(c->rx+c->rw*0.5f)*TILEF, rcy=(c->ry+c->rh*0.5f)*TILEF;
  if(B->flash>0.0f) B->flash-=dt;
  float hpr=B->hp/B->maxhp;
  int ph=hpr>0.66f?1:(hpr>0.33f?2:3);
  if(ph!=B->phase){ B->phase=ph; snd_play(SFX_PHASE); add_trauma(0.3f); B->flash=0.12f; g_fringeFx=0.5f;
    spawn_ring(B->pos.x,B->pos.y,br,500.0f,0.4f,1.0f,0.4f,0.4f);
    spawn_ring(B->pos.x,B->pos.y,br*0.4f,720.0f,0.5f,1.0f,1.0f,1.0f);
    burst(B->pos.x,B->pos.y,18,300.0f,1.0f,0.5f,0.3f); }
  float dx=g_player.pos.x-B->pos.x, dy=g_player.pos.y-B->pos.y;
  float dist=f_sqrt(dx*dx+dy*dy); if(dist<0.01f)dist=0.01f;

  if(B->type==0){ /* === THE CORE: 탄막형 (§9.1) === */
    float mv=ph==3?60.0f:30.0f;
    B->pos.x+=dx/dist*mv*dt; B->pos.y+=dy/dist*mv*dt;
    if(B->pos.x<mnx)B->pos.x=mnx; if(B->pos.x>mxx)B->pos.x=mxx;
    if(B->pos.y<mny)B->pos.y=mny; if(B->pos.y>mxy)B->pos.y=mxy;
    B->t1+=dt;
    float ringInt=ph==3?1.5f:2.5f;
    if(B->t1>=ringInt){ B->t1=0.0f; B->segAng+=0.4f; /* 16발 방사 링 (회전 오프셋) */
      for(k=0;k<16;k++){ float a=B->segAng+(float)k*0.3926991f;
        spawn_ebul(B->pos.x,B->pos.y,f_cos(a),f_sin(a),220.0f,4.0f); }
      snd_play(SFX_SHOOT);
    }
    if(ph>=2){
      if(B->t2<dt){ B->ldir.x=dx/dist; B->ldir.y=dy/dist; } /* 사이클 시작: 조준 */
      B->t2+=dt;
      if(B->t2>=0.8f&&B->t2<1.2f){ /* 발사: 150°/s 스윕 */
        vrot(&B->ldir,2.618f*dt);
        if(beam_hit(B->pos.x,B->pos.y,B->ldir.x,B->ldir.y,7.0f,g_player.pos.x,g_player.pos.y,g_player.radius,700.0f)) hurt_player(2.0f);
        if(ph==3&&beam_hit(B->pos.x,B->pos.y,-B->ldir.x,-B->ldir.y,7.0f,g_player.pos.x,g_player.pos.y,g_player.radius,700.0f)) hurt_player(2.0f);
      }
      if(B->t2>=5.0f) B->t2=0.0f;
      B->t3+=dt;
      if(B->t3>=6.0f){ B->t3=0.0f; /* 헌터 2 소환 */
        spawn_enemy(0,B->pos.x-60.0f,B->pos.y,0); spawn_enemy(0,B->pos.x+60.0f,B->pos.y,0); }
    }
  } else if(B->type==1){ /* === THE WARDEN: 돌진/공간압축형 (§9.2) === */
    int nseg=ph==3?3:2;
    float segRate=(ph==1?90.0f:(ph==2?130.0f:70.0f))*0.0174533f;
    B->segAng+=segRate*dt;
    for(k=0;k<nseg;k++){ /* 공전 세그먼트 접촉뎀 1 */
      float a=B->segAng+(float)k*6.2831853f/(float)nseg;
      float sx=B->pos.x+f_cos(a)*130.0f, sy=B->pos.y+f_sin(a)*130.0f;
      float sdx=sx-g_player.pos.x, sdy=sy-g_player.pos.y, rr=16.0f+g_player.radius;
      if(sdx*sdx+sdy*sdy<rr*rr) hurt_player(1.0f);
    }
    float idleT=ph==1?1.6f:(ph==2?1.2f:0.9f);
    if(B->dashState==0){ /* 부유 + 돌진 준비 */
      B->pos.x+=dx/dist*40.0f*dt; B->pos.y+=dy/dist*40.0f*dt;
      B->stateT+=dt;
      if(B->stateT>=idleT){ B->stateT=0.0f; B->dashState=1; B->dashDir.x=dx/dist; B->dashDir.y=dy/dist; }
    } else if(B->dashState==1){ /* 0.7s 조준 텔레그래프 */
      B->stateT+=dt;
      if(B->stateT>=0.7f){ B->stateT=0.0f; B->dashState=2; snd_play(SFX_DASH); }
    } else if(B->dashState==2){ /* 돌진 900px/s + 잔류 트레일 */
      B->pos.x+=B->dashDir.x*900.0f*dt; B->pos.y+=B->dashDir.y*900.0f*dt;
      B->trailT-=dt;
      if(B->trailT<=0.0f){ B->trailT=0.04f;
        for(k=0;k<MAXTRAIL;k++) if(!g_trail[k].active){ g_trail[k].active=1; g_trail[k].pos=B->pos; g_trail[k].life=0.6f; break; } }
      if(B->pos.x<mnx||B->pos.x>mxx||B->pos.y<mny||B->pos.y>mxy){
        if(B->pos.x<mnx)B->pos.x=mnx; if(B->pos.x>mxx)B->pos.x=mxx;
        if(B->pos.y<mny)B->pos.y=mny; if(B->pos.y>mxy)B->pos.y=mxy;
        B->dashState=3; B->stateT=0.0f; add_trauma(0.4f); snd_play(SFX_HIT);
        burst(B->pos.x,B->pos.y,14,260.0f,1.0f,0.6f,0.2f);
      }
    } else { B->stateT+=dt; if(B->stateT>=0.5f){ B->stateT=0.0f; B->dashState=0; } }
    if(ph>=2){ /* 회전 레이저 암 */
      int narm=ph==3?3:2;
      float armRate=(ph==2?45.0f:70.0f)*0.0174533f;
      vrot(&B->wdir,armRate*dt);
      for(k=0;k<narm;k++){ vec2 ad=B->wdir; vrot(&ad,(float)k*6.2831853f/(float)narm);
        if(beam_hit(B->pos.x,B->pos.y,ad.x,ad.y,7.0f,g_player.pos.x,g_player.pos.y,g_player.radius,700.0f)) hurt_player(2.0f); }
    }
    if(ph==3){ /* 압축 격노: 안전 반경 수축 + 상시 약한 셰이크 */
      if(B->shrink>180.0f) B->shrink-=8.0f*dt;
      if(g_trauma<0.08f) g_trauma=0.08f;
      float pdx=g_player.pos.x-rcx, pdy=g_player.pos.y-rcy;
      if(pdx*pdx+pdy*pdy>B->shrink*B->shrink) hurt_player(1.0f);
    }
  } else if(B->type==2){ /* === THE NEXUS: 노드망 빔케이지 존-컨트롤 (§9.3) === */
    int kk2; float nx3[3], ny3[3];
    if(B->shrink>150.0f) B->shrink=150.0f; /* 스폰 init 정규화 → 노드 반경 */
    float nd=B->shrink;
    B->segAng+=(ph==3?1.05f:(ph==2?0.79f:0.61f))*dt;
    B->pos.x+=dx/dist*25.0f*dt; B->pos.y+=dy/dist*25.0f*dt;
    if(B->pos.x<mnx)B->pos.x=mnx; if(B->pos.x>mxx)B->pos.x=mxx;
    if(B->pos.y<mny)B->pos.y=mny; if(B->pos.y>mxy)B->pos.y=mxy;
    for(kk2=0;kk2<3;kk2++){ float a=B->segAng+(float)kk2*2.0943951f; nx3[kk2]=B->pos.x+f_cos(a)*nd; ny3[kk2]=B->pos.y+f_sin(a)*nd; }
    float halfw=7.0f; /* 전 페이즈 동일 — ph3 압박은 케이지 수축으로 (보이지 않는 폭증 제거) */
    for(kk2=0;kk2<3;kk2++){ int k3=(kk2+1)%3; /* 회전 삼각 빔케이지 (3변) */
      float ex=nx3[k3]-nx3[kk2], ey=ny3[k3]-ny3[kk2], el=f_sqrt(ex*ex+ey*ey); if(el<0.01f)el=0.01f;
      if(beam_hit(nx3[kk2],ny3[kk2],ex/el,ey/el,halfw,g_player.pos.x,g_player.pos.y,g_player.radius,el)) hurt_player(2.0f); }
    if(ph>=2){ B->t1+=dt; /* 노드 방사 링 (밀도 완화: 노드당 8발) */
      if(B->t1>=(ph==3?3.6f:2.2f)){ B->t1=0.0f; int b2;
        for(kk2=0;kk2<3;kk2++) for(b2=0;b2<8;b2++){ float a=(float)b2*0.7853982f; spawn_ebul(nx3[kk2],ny3[kk2],f_cos(a),f_sin(a),240.0f,4.0f); }
        snd_play(SFX_PHASE); } } /* 36발→24발, 위협 사운드 */
    if(ph==3){ if(B->shrink>70.0f) B->shrink-=12.0f*dt; if(g_trauma<0.06f) g_trauma=0.06f; /* 수축 18→12px/s, 본체 링 16→12발 */
      B->t2+=dt; if(B->t2>=1.6f){ B->t2=0.0f; int b2; for(b2=0;b2<12;b2++){ float a=B->segAng+(float)b2*0.5235988f; spawn_ebul(B->pos.x,B->pos.y,f_cos(a),f_sin(a),220.0f,4.0f); } } }
    B->t3+=dt; if(B->t3>=8.0f){ B->t3=0.0f; int tc=0,tt; for(tt=0;tt<MAXENE;tt++) if(g_ene[tt].active&&g_ene[tt].type==1)tc++; /* 터릿 소환: 최대 2, 방 경계 클램프 */
      if(tc<2){ float tsx=nx3[0], tsy=ny3[0]; if(tsx<mnx)tsx=mnx; if(tsx>mxx)tsx=mxx; if(tsy<mny)tsy=mny; if(tsy>mxy)tsy=mxy; spawn_enemy(1,tsx,tsy,0); } }
    for(kk2=0;kk2<3;kk2++){ float ndx=nx3[kk2]-g_player.pos.x, ndy=ny3[kk2]-g_player.pos.y, rr=14.0f+g_player.radius; if(ndx*ndx+ndy*ndy<rr*rr) hurt_player(1.0f); }
  }
  /* 트레일 수명/접촉 */
  for(k=0;k<MAXTRAIL;k++){ if(!g_trail[k].active) continue;
    g_trail[k].life-=dt; if(g_trail[k].life<=0.0f){ g_trail[k].active=0; continue; }
    float tdx=g_trail[k].pos.x-g_player.pos.x, tdy=g_trail[k].pos.y-g_player.pos.y, rr=14.0f+g_player.radius;
    if(tdx*tdx+tdy*tdy<rr*rr) hurt_player(1.0f);
  }
  /* 보스 본체 접촉뎀 2 */
  if(g_boss.active&&dist<br+g_player.radius) hurt_player(2.0f);
}
static void consume_latches(void){ g_wantDash=0; g_wantEmp=0; } /* sim 스텝 끝에서만 소거 */
static void spawn_minib(float x,float y,float ux,float uy,float dmg){
  int i; for(i=0;i<MAXBUL;i++) if(!g_bul[i].active){ Bullet*b=&g_bul[i];
    b->active=1; b->pos.x=x; b->pos.y=y; b->vel.x=ux*500.0f; b->vel.y=uy*500.0f;
    b->life=0.5f; b->dmg=dmg; b->r=3.0f; b->pierce=0; b->bounce=0; b->mini=1; b->lastHit=0; return; }
}

static void combat_update(float dt){
  int i,j;
  g_time+=dt;
  g_fireCd-=dt; if(g_pIfr>0.0f) g_pIfr-=dt;
  if(g_trauma>0.0f){ g_trauma-=TRAUMA_DECAY*dt; if(g_trauma<0.0f)g_trauma=0.0f; }

  /* 광란(FRENZY): 홀드 시 연사속도 램프, 떼면 감쇠 (캡 +1.5) */
  if(g_mod[M_FRENZY]){
    if(g_mouseDown){ float fmax=0.5f*(float)g_mod[M_FRENZY]; if(fmax>1.5f)fmax=1.5f; g_frenzy+=dt; if(g_frenzy>fmax)g_frenzy=fmax; }
    else { g_frenzy-=2.0f*dt; if(g_frenzy<0.0f)g_frenzy=0.0f; }
  } else g_frenzy=0.0f;
  /* 발사 (LMB 홀드) — 모듈 수식(§6) + 총구 플래시 + 카메라 킥 */
  if(g_mouseDown && g_fireCd<=0.0f){
    g_fireCd=FIRE_INT/(1.0f+0.20f*(float)g_mod[M_RAPID]+g_frenzy);
    float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
    float dx=mwx-g_player.pos.x, dy=mwy-g_player.pos.y, l=f_sqrt(dx*dx+dy*dy); if(l<0.001f)l=1.0f;
    dx/=l; dy/=l;
    float bx=g_player.pos.x+dx*16.0f, by=g_player.pos.y+dy*16.0f;
    int nsh=1+(g_mod[M_MULTI]>6?6:g_mod[M_MULTI]), s;
    float spr=12.0f*(float)(nsh-1); if(spr>60.0f)spr=60.0f; spr*=0.0174533f;
    float bspd=BUL_SPEED*(1.0f+0.30f*(float)g_mod[M_SWIFT]), blife=BUL_LIFE*(1.0f+0.30f*(float)g_mod[M_SWIFT]);
    float critc=0.15f*(float)g_mod[M_CRIT]; if(critc>1.0f)critc=1.0f;
    g_shotCount++;
    float psp=f_sqrt(g_player.vel.x*g_player.vel.x+g_player.vel.y*g_player.vel.y); /* KINETIC: 이동속도 */
    int big=(g_mod[M_OVERLOAD]&&(g_shotCount%10)==0); /* 과부하: 매 10발 대형탄 */
    for(s=0;s<nsh;s++){
      float ang=nsh>1?(-spr*0.5f+spr*(float)s/(float)(nsh-1)):0.0f;
      float ca=f_cos(ang), sa=f_sin(ang);
      float ux=dx*ca-dy*sa, uy=dx*sa+dy*ca;
      float dmg=BUL_DMG*(1.0f+0.25f*(float)g_mod[M_POWER]), rr=BUL_R;
      if(g_mod[M_VENGEANCE]){ float miss=1.0f-g_pHP/g_pMaxHP; if(miss<0.0f)miss=0.0f; dmg*=1.0f+0.20f*(float)g_mod[M_VENGEANCE]*(0.25f+0.75f*miss); } /* 보복: 저HP=뎀↑ (풀피 +5%, 저체 +17%/스택) */
      if(g_mod[M_KINETIC]){ float sp01=psp/MAXSPEED; if(sp01>1.0f)sp01=1.0f; dmg*=1.0f+0.22f*(float)g_mod[M_KINETIC]*(0.4f+0.6f*sp01); } /* 운동: 이속=뎀↑ (정지 +9%, 전속 +22%/스택) */
      if(g_mod[M_GLASSCANNON]) dmg*=1.0f+0.60f*(float)(g_mod[M_GLASSCANNON]>2?2:g_mod[M_GLASSCANNON]); /* 글래스캐논: +60%/스택(캡2) */
      if(critc>0.0f&&rnd01()<critc){ dmg*=2.0f; rr*=1.3f; }
      if(big){ dmg*=4.0f; rr*=2.0f; }
      for(i=0;i<MAXBUL;i++) if(!g_bul[i].active){ Bullet*b=&g_bul[i];
        b->active=1; b->pos.x=bx; b->pos.y=by; b->vel.x=ux*bspd; b->vel.y=uy*bspd;
        b->life=blife; b->dmg=dmg; b->r=rr;
        b->pierce=(unsigned char)(g_mod[M_PIERCE]>200?200:g_mod[M_PIERCE]);
        b->bounce=(unsigned char)(g_mod[M_BOUNCE]*2>200?200:g_mod[M_BOUNCE]*2);
        b->mini=0; b->lastHit=0; break; }
    }
    spawn_part(bx,by,dx*120.0f+fxsym()*40.0f,dy*120.0f+fxsym()*40.0f,0.08f,4.0f,1.0f,1.0f,0.9f);
    spawn_part(bx,by,dx*80.0f+fxsym()*60.0f,dy*80.0f+fxsym()*60.0f,0.06f,3.0f,0.5f,1.0f,1.0f);
    g_kick.x-=dx*2.2f; g_kick.y-=dy*2.2f; /* 반동 킥 */
    snd_play(SFX_SHOOT);
  }

  /* 블링크 (RMB, docs/06 §2.1): 마우스 방향 최대 180px, 벽 직전까지 */
  if(g_blinkCd>0.0f) g_blinkCd-=dt;
  if(g_empCd>0.0f) g_empCd-=dt;
  if(g_rmbPressed && g_blinkCd<=0.0f){
    g_rmbPressed=0; g_blinkCd=BLINK_CD;
    float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
    float dx=mwx-g_player.pos.x, dy=mwy-g_player.pos.y, l=f_sqrt(dx*dx+dy*dy); if(l<0.001f)l=1.0f;
    dx/=l; dy/=l;
    float px=g_player.pos.x, py=g_player.pos.y, r=g_player.radius*0.7f;
    float bx=px, by=py, s;
    for(s=8.0f;s<=BLINK_RANGE;s+=8.0f){
      float qx=px+dx*s, qy=py+dy*s;
      if(is_wall_w(qx-r,qy)||is_wall_w(qx+r,qy)||is_wall_w(qx,qy-r)||is_wall_w(qx,qy+r)) break;
      bx=qx; by=qy;
    }
    burst(px,py,8,160.0f,0.4f,0.95f,1.0f);
    spawn_after(px+(bx-px)*0.25f,py+(by-py)*0.25f);
    spawn_after(px+(bx-px)*0.5f,py+(by-py)*0.5f);
    spawn_after(px+(bx-px)*0.75f,py+(by-py)*0.75f);
    g_player.pos.x=bx; g_player.pos.y=by;
    g_player.vel.x=dx*MAXSPEED; g_player.vel.y=dy*MAXSPEED;
    if(g_pIfr<0.10f) g_pIfr=0.10f;
    burst(bx,by,10,180.0f,0.6f,1.0f,1.0f);
    spawn_ring(bx,by,8.0f,320.0f,0.25f,0.5f,1.0f,1.0f);
    snd_play(SFX_DASH);
  }

  /* EMP 버스트 (Q, docs/06 §2.2): AoE 데미지+넉백+적탄 소거 */
  if(g_wantEmp && g_empCd<=0.0f){
    g_wantEmp=0; g_empCd=EMP_CD;
    for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
      float dx=e->pos.x-g_player.pos.x, dy=e->pos.y-g_player.pos.y, d2=dx*dx+dy*dy, rr=EMP_R+ene_radius(e->type);
      if(d2<rr*rr){ float d=f_sqrt(d2); if(d<0.01f)d=0.01f;
        if(e->eshield){ e->eshield=0; e->at=0; }
        else { e->hp-=EMP_DMG; e->flash=0.08f; }
        e->kvel.x+=dx/d*420.0f; e->kvel.y+=dy/d*420.0f;
        if(e->hp<=0.0f) kill_enemy(j);
      }
    }
    for(j=0;j<MAXEBUL;j++){ if(!g_ebul[j].active) continue;
      float dx=g_ebul[j].pos.x-g_player.pos.x, dy=g_ebul[j].pos.y-g_player.pos.y;
      if(dx*dx+dy*dy<EMP_R*EMP_R){ g_ebul[j].active=0;
        spawn_part(g_ebul[j].pos.x,g_ebul[j].pos.y,dx*1.5f,dy*1.5f,0.18f,3.0f,0.5f,0.9f,1.0f); }
    }
    if(g_boss.active){ float bdx=g_boss.pos.x-g_player.pos.x, bdy=g_boss.pos.y-g_player.pos.y, rr=EMP_R+boss_radius();
      if(bdx*bdx+bdy*bdy<rr*rr) boss_hit(EMP_DMG); }
    add_trauma(0.35f); g_hitstop=0.05f;
    spawn_ring(g_player.pos.x,g_player.pos.y,20.0f,600.0f,0.35f,0.3f,0.9f,1.0f);
    spawn_ring(g_player.pos.x,g_player.pos.y,10.0f,900.0f,0.25f,1.0f,1.0f,1.0f);
    snd_play(SFX_PHASE);
  }

  /* 플레이어 발사체 */
  for(i=0;i<MAXBUL;i++){ if(!g_bul[i].active) continue; Bullet*b=&g_bul[i];
    /* 유도 (선회율 90+60/stk°/s, 상한 360 — §7 가드) */
    if(g_mod[M_HOMING]&&!b->mini){
      int bj=-1; float bd2=420.0f*420.0f;
      for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue;
        float ddx=g_ene[j].pos.x-b->pos.x, ddy=g_ene[j].pos.y-b->pos.y, d2=ddx*ddx+ddy*ddy;
        if(d2<bd2){ bd2=d2; bj=j; } }
      if(bj>=0){
        float vl=f_sqrt(b->vel.x*b->vel.x+b->vel.y*b->vel.y); if(vl<0.001f)vl=1.0f;
        float ux=b->vel.x/vl, uy=b->vel.y/vl;
        float d=f_sqrt(bd2); if(d<0.01f)d=0.01f;
        float tx2=(g_ene[bj].pos.x-b->pos.x)/d, ty2=(g_ene[bj].pos.y-b->pos.y)/d;
        float cross=ux*ty2-uy*tx2, dot=ux*tx2+uy*ty2;
        if(dot<0.9995f){
          float turn=90.0f+60.0f*(float)(g_mod[M_HOMING]-1); if(turn>360.0f)turn=360.0f;
          float a=turn*0.0174533f*dt; if(cross<0.0f)a=-a;
          float ca=f_cos(a), sa=f_sin(a);
          float nx2=ux*ca-uy*sa, ny2=ux*sa+uy*ca;
          b->vel.x=nx2*vl; b->vel.y=ny2*vl;
        }
      }
    }
    float prevx=b->pos.x, prevy=b->pos.y;
    b->pos.x+=b->vel.x*dt; b->pos.y+=b->vel.y*dt; b->life-=dt;
    if(b->life<=0.0f){ b->active=0; continue; }
    if(try_break_crate(b->pos.x,b->pos.y)){ b->active=0; continue; } /* 크레이트 파괴 */
    if(is_wall_w(b->pos.x,b->pos.y)){
      if(b->bounce>0){ /* 반사: 수명은 계속 소모 (degenerate 가드 §7) */
        b->bounce--;
        int fx=is_wall_w(b->pos.x,prevy), fy=is_wall_w(prevx,b->pos.y);
        if(fx) b->vel.x=-b->vel.x;
        if(fy) b->vel.y=-b->vel.y;
        if(!fx&&!fy){ b->vel.x=-b->vel.x; b->vel.y=-b->vel.y; }
        b->pos.x=prevx; b->pos.y=prevy; b->lastHit=0;
        spawn_part(b->pos.x,b->pos.y,0,0,0.1f,2.5f,0.4f,0.9f,1.0f);
      } else {
        spawn_part(b->pos.x,b->pos.y,-b->vel.x*0.1f,-b->vel.y*0.1f,0.12f,3.0f,0.4f,0.9f,1.0f);
        b->active=0; continue; }
    }
    for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
      if(b->lastHit==(unsigned char)(j+1)) continue; /* 관통 중복히트 방지 */
      float rr=ene_radius(e->type)+b->r, ddx=b->pos.x-e->pos.x, ddy=b->pos.y-e->pos.y;
      if(ddx*ddx+ddy*ddy<rr*rr){
        float bl=f_sqrt(b->vel.x*b->vel.x+b->vel.y*b->vel.y); if(bl<0.001f)bl=1.0f;
        if(e->eshield){ /* 보호막 어픽스: 1회 피격 무효 */
          e->eshield=0; e->at=0;
          e->kvel.x+=b->vel.x/bl*60.0f; e->kvel.y+=b->vel.y/bl*60.0f;
          spawn_ring(e->pos.x,e->pos.y,ene_radius(e->type)+6.0f,200.0f,0.2f,0.5f,0.9f,1.0f);
          snd_play(SFX_HIT); b->active=0; break;
        }
        e->hp-=b->dmg; e->flash=0.07f;
        e->kvel.x+=b->vel.x/bl*130.0f; e->kvel.y+=b->vel.y/bl*130.0f; /* 넉백 임펄스 */
        burst(b->pos.x,b->pos.y,5,180.0f,1.0f,1.0f,1.0f);
        snd_play(SFX_HIT);
        if(g_mod[M_EXPLODE]&&!b->mini) explode_at(b->pos.x,b->pos.y,b->dmg);
        if(g_mod[M_CHAINLIGHT]&&!b->mini) arc_chain(e->pos.x,e->pos.y,j,b->dmg*(0.35f+0.10f*(float)(g_mod[M_CHAINLIGHT]-1))); /* 아크 체인 */
        if(g_mod[M_SPLIT]&&!b->mini){ /* 분열탄 (레어): 미니 2발 수직 분기 */
          float ux=b->vel.x/bl, uy=b->vel.y/bl;
          spawn_minib(b->pos.x,b->pos.y,-uy,ux,b->dmg*0.4f);
          spawn_minib(b->pos.x,b->pos.y,uy,-ux,b->dmg*0.4f);
        }
        if(e->hp<=0.0f) kill_enemy(j);
        if(b->pierce>0){ b->pierce--; b->lastHit=(unsigned char)(j+1); }
        else b->active=0;
        break;
      }
    }
    /* 보스 피격 */
    if(b->active&&g_boss.active){
      float rr=boss_radius()+b->r, bdx=b->pos.x-g_boss.pos.x, bdy=b->pos.y-g_boss.pos.y;
      if(bdx*bdx+bdy*bdy<rr*rr){
        float bl=f_sqrt(b->vel.x*b->vel.x+b->vel.y*b->vel.y); if(bl<0.001f)bl=1.0f;
        burst(b->pos.x,b->pos.y,5,180.0f,1.0f,1.0f,1.0f);
        snd_play(SFX_HIT);
        if(g_mod[M_EXPLODE]&&!b->mini) explode_at(b->pos.x,b->pos.y,b->dmg);
        if(g_mod[M_SPLIT]&&!b->mini){
          float ux=b->vel.x/bl, uy=b->vel.y/bl;
          spawn_minib(b->pos.x,b->pos.y,-uy,ux,b->dmg*0.4f);
          spawn_minib(b->pos.x,b->pos.y,uy,-ux,b->dmg*0.4f);
        }
        b->active=0;
        boss_hit(b->dmg);
      }
    }
  }

  /* 적 AI */
  for(i=0;i<MAXENE;i++){ if(!g_ene[i].active) continue; Enemy*e=&g_ene[i];
    if(e->spawn>0.0f){ e->spawn-=dt; continue; } /* 스폰 텔레그래프 중 */
    if(e->flash>0.0f) e->flash-=dt;
    float r=ene_radius(e->type), spd=ene_speed(e->type);
    if(e->affix&2) spd*=1.4f; /* 신속 (Swift) */
    float dx=g_player.pos.x-e->pos.x, dy=g_player.pos.y-e->pos.y, dist=f_sqrt(dx*dx+dy*dy); if(dist<0.01f)dist=0.01f;
    if(g_mod[M_MAGNET]){ float ar=80.0f+20.0f*(float)g_mod[M_MAGNET]; if(dist<ar) spd*=0.72f; } /* 자력 소형 감속 오라 (§7) */
    e->t+=dt;
    /* 어픽스 타이머: 보호막 재충전 1.5s / 자가분열 4s */
    if((e->affix&1)&&!e->eshield){ e->at+=dt; if(e->at>=1.5f) e->eshield=1; }
    if(e->affix&8){ e->at2+=dt;
      if(e->at2>=4.0f){ e->at2=0;
        int na=0; for(j=0;j<MAXENE;j++) if(g_ene[j].active) na++;
        if(na<64){ int ci=spawn_enemy(e->type,e->pos.x+rndsym()*30.0f,e->pos.y+rndsym()*30.0f,0);
          if(ci>=0) g_ene[ci].hp*=0.5f; }
      } }
    if(e->affix&16){ /* 자성 (Magnetic): 플레이어 흡인 오라 */
      if(dist<260.0f){ g_player.vel.x-=dx/dist*140.0f*dt; g_player.vel.y-=dy/dist*140.0f*dt; }
    }
    /* 넉백 감쇠 */
    float kx=e->kvel.x, ky=e->kvel.y;
    e->kvel.x-=e->kvel.x*9.0f*dt; e->kvel.y-=e->kvel.y*9.0f*dt;
    if(e->type==0||e->type==3||e->type==4){ /* 헌터/포크/샤드: 추격 (+헌터 동족 분리) */
      float sx=0,sy=0;
      if(e->type==0) for(j=0;j<MAXENE;j++){ if(j==i||!g_ene[j].active||g_ene[j].type!=0) continue;
        float ox=e->pos.x-g_ene[j].pos.x, oy=e->pos.y-g_ene[j].pos.y, od2=ox*ox+oy*oy;
        if(od2<900.0f&&od2>0.01f){ float od=f_sqrt(od2); sx+=ox/od; sy+=oy/od; } }
      float vx=dx/dist*spd+sx*60.0f+kx, vy=dy/dist*spd+sy*60.0f+ky;
      float nx=e->pos.x+vx*dt, ex2=nx+(vx>0?r:-r);
      if(!(is_wall_w(ex2,e->pos.y-r*0.6f)||is_wall_w(ex2,e->pos.y+r*0.6f))) e->pos.x=nx;
      float ny=e->pos.y+vy*dt, ey2=ny+(vy>0?r:-r);
      if(!(is_wall_w(e->pos.x-r*0.6f,ey2)||is_wall_w(e->pos.x+r*0.6f,ey2))) e->pos.y=ny;
    } else if(e->type==1){ /* 터릿: 거리유지 + 발사(0.3s 텔레그래프) */
      float dir=0.0f; if(dist<300.0f) dir=-1.0f; else if(dist>340.0f) dir=1.0f;
      float vx=dx/dist*spd*dir+kx, vy=dy/dist*spd*dir+ky;
      float nx=e->pos.x+vx*dt, ex2=nx+(vx>0?r:-r);
      if(!(is_wall_w(ex2,e->pos.y-r*0.6f)||is_wall_w(ex2,e->pos.y+r*0.6f))) e->pos.x=nx;
      float ny=e->pos.y+vy*dt, ey2=ny+(vy>0?r:-r);
      if(!(is_wall_w(e->pos.x-r*0.6f,ey2)||is_wall_w(e->pos.x+r*0.6f,ey2))) e->pos.y=ny;
      e->t2+=dt;
      float fireInt=(e->affix&2)?1.14f:1.6f; /* 신속: 공속 +40% */
      if(e->t2>=fireInt){ e->t2=0.0f;
        for(j=0;j<MAXEBUL;j++) if(!g_ebul[j].active){ g_ebul[j].active=1; g_ebul[j].pos=e->pos; g_ebul[j].vel.x=dx/dist*EBUL_SPEED; g_ebul[j].vel.y=dy/dist*EBUL_SPEED; g_ebul[j].life=2.5f; break; }
      }
    } else if(e->type==5){ /* LANCER: 텔레그래프 돌진 스트라이커 */
      float sm=(e->affix&2)?0.7f:1.0f, t2=e->t2/sm;
      e->t2+=dt;
      if(t2<1.1f){ /* 스토크: 거리유지 측면 슬라이드 + 돌진방향 상시 조준 */
        e->vel.x=dx/dist*520.0f; e->vel.y=dy/dist*520.0f;
        float px=-dy/dist, py=dx/dist, dir=dist<180.0f?-0.5f:(dist>250.0f?0.7f:0.0f);
        float vx=(dx/dist*dir+px*0.7f)*spd+kx, vy=(dy/dist*dir+py*0.7f)*spd+ky;
        float nx=e->pos.x+vx*dt, ex2=nx+(vx>0?r:-r);
        if(!(is_wall_w(ex2,e->pos.y-r*0.6f)||is_wall_w(ex2,e->pos.y+r*0.6f))) e->pos.x=nx;
        float ny=e->pos.y+vy*dt, ey2=ny+(vy>0?r:-r);
        if(!(is_wall_w(e->pos.x-r*0.6f,ey2)||is_wall_w(e->pos.x+r*0.6f,ey2))) e->pos.y=ny;
      } else if(t2<1.45f){ /* 돌진: 520px/s + 벽 1회 반사 (코너 진동 방지: ±0.6r 2점) */
        float nx=e->pos.x+e->vel.x*dt, ex2=nx+(e->vel.x>0?r:-r);
        if(is_wall_w(ex2,e->pos.y-r*0.6f)||is_wall_w(ex2,e->pos.y+r*0.6f)) e->vel.x=-e->vel.x; else e->pos.x=nx;
        float ny=e->pos.y+e->vel.y*dt, ey2=ny+(e->vel.y>0?r:-r);
        if(is_wall_w(e->pos.x-r*0.6f,ey2)||is_wall_w(e->pos.x+r*0.6f,ey2)) e->vel.y=-e->vel.y; else e->pos.y=ny;
      } else if(t2<1.95f){ e->vel.x*=0.86f; e->vel.y*=0.86f; e->pos.x+=e->vel.x*dt; e->pos.y+=e->vel.y*dt; }
      else { e->t2=0.0f; }
    } else if(e->type==6){ /* WEAVER: 궤도 비행 + 기뢰 설치 (g_boom 재활용) */
      float px=-dy/dist, py=dx/dist, dir=dist<220.0f?1.0f:(dist>300.0f?-1.0f:0.0f);
      float vx=(dx/dist*dir+px)*spd+kx, vy=(dy/dist*dir+py)*spd+ky;
      float nx=e->pos.x+vx*dt, ex2=nx+(vx>0?r:-r);
      if(!(is_wall_w(ex2,e->pos.y-r*0.6f)||is_wall_w(ex2,e->pos.y+r*0.6f))) e->pos.x=nx; else e->kvel.x*=0.3f; /* 벽 스터터 억제 */
      float ny=e->pos.y+vy*dt, ey2=ny+(vy>0?r:-r);
      if(!(is_wall_w(e->pos.x-r*0.6f,ey2)||is_wall_w(e->pos.x+r*0.6f,ey2))) e->pos.y=ny; else e->kvel.y*=0.3f;
      e->t2+=dt;
      float dropInt=(e->affix&2)?1.6f:2.3f;
      if(e->t2>=dropInt){ e->t2=0.0f; int q; for(q=0;q<MAXBOOM;q++) if(!g_boom[q].active){
        g_boom[q].active=1; g_boom[q].pos=e->pos; g_boom[q].t=g_boom[q].t0=1.4f;
        spawn_ring(e->pos.x,e->pos.y,10.0f,260.0f,0.25f,1.0f,0.4f,0.2f); break; } }
    } else if(e->type==2){ /* 리코셰: 직선+반사, 1.5s 방향 재설정 */
      if(e->t>=1.5f){ e->t=0.0f; float rx=rndsym(), ry=rndsym();
        if((xrnd()%100)<30){ rx=dx/dist; ry=dy/dist; }
        float l=f_sqrt(rx*rx+ry*ry); if(l<0.01f)l=1.0f; e->vel.x=rx/l*spd; e->vel.y=ry/l*spd; }
      float nx=e->pos.x+(e->vel.x+kx)*dt, ex2=nx+(e->vel.x>0?r:-r);
      if(is_wall_w(ex2,e->pos.y)) e->vel.x=-e->vel.x; else e->pos.x=nx;
      float ny=e->pos.y+(e->vel.y+ky)*dt, ey2=ny+(e->vel.y>0?r:-r);
      if(is_wall_w(e->pos.x,ey2)) e->vel.y=-e->vel.y; else e->pos.y=ny;
    }
    /* 접촉 데미지 (+ 가시 모듈: 접촉 적 반격) */
    float cr=r+g_player.radius;
    if(dx*dx+dy*dy<cr*cr){
      if(g_mod[M_THORNS]&&g_thornT<=0.0f){
        g_thornT=0.25f;
        e->hp-=15.0f*(float)g_mod[M_THORNS]; e->flash=0.07f;
        e->kvel.x-=dx/dist*260.0f; e->kvel.y-=dy/dist*260.0f;
        burst(e->pos.x,e->pos.y,6,170.0f,0.4f,1.0f,0.6f);
        if(e->hp<=0.0f){ kill_enemy(i); continue; }
      }
      hurt_player(1.0f);
    }
  }

  /* 보스 (§9) */
  boss_update(dt);

  /* 적 발사체 */
  for(i=0;i<MAXEBUL;i++){ if(!g_ebul[i].active) continue; Bullet*b=&g_ebul[i];
    b->pos.x+=b->vel.x*dt; b->pos.y+=b->vel.y*dt; b->life-=dt;
    if(b->life<=0.0f||is_wall_w(b->pos.x,b->pos.y)){ b->active=0; continue; }
    if(g_pIfr>0.0f||g_dashT>0.0f) continue; /* 무적/대시 중 통과 (§8) */
    float ddx=b->pos.x-g_player.pos.x, ddy=b->pos.y-g_player.pos.y, rr=g_player.radius+6.0f;
    if(ddx*ddx+ddy*ddy<rr*rr){ b->active=0; hurt_player(1.0f); }
  }

  /* 레어 모듈 타이머: 재생/수호막 + 가시 틱 */
  if(g_mod[M_REGEN]){ g_regenT+=dt;
    if(g_regenT>=25.0f){ g_regenT=0.0f; /* 비스택: GDD §7 '25초마다 1HP' (회복 희소성 §13) */
      if(g_pHP<g_pMaxHP){ g_pHP+=1.0f; spawn_ring(g_player.pos.x,g_player.pos.y,10.0f,180.0f,0.3f,0.3f,1.0f,0.5f); } } }
  if(g_mod[M_AEGIS]&&!g_shieldUp){ g_shieldT+=dt;
    if(g_shieldT>=15.0f){ g_shieldUp=1; snd_play(SFX_PICKUP); } }
  if(g_thornT>0.0f) g_thornT-=dt;

  /* BITS 픽업 (§13: 자력 70px ×자력모듈, 접촉 획득) */
  for(i=0;i<MAXPICK;i++){ if(!g_pick[i].active) continue; Pickup*p=&g_pick[i];
    p->t+=dt;
    float dx=g_player.pos.x-p->pos.x, dy=g_player.pos.y-p->pos.y, d2=dx*dx+dy*dy;
    float mag=p->homing?1e9f:70.0f*(1.0f+1.2f*(float)g_mod[M_MAGNET]);
    if(d2<mag*mag&&d2>1.0f){ float d=f_sqrt(d2); p->vel.x+=dx/d*1400.0f*dt; p->vel.y+=dy/d*1400.0f*dt; }
    p->vel.x-=p->vel.x*4.0f*dt; p->vel.y-=p->vel.y*4.0f*dt;
    p->pos.x+=p->vel.x*dt; p->pos.y+=p->vel.y*dt;
    if(d2<(g_player.radius+8.0f)*(g_player.radius+8.0f)){
      p->active=0;
      if(p->kind==1){ /* 모듈 오브: 랜덤 커먼 1개 즉시 설치 (§13) */
        int m=-1, tr2;
        for(tr2=0;tr2<20;tr2++){ int c2=(int)(xrnd()%MOD_COMMON); if(!mod_capped(c2)){ m=c2; break; } }
        if(m>=0){ g_mod[m]++;
          spawn_ring(g_player.pos.x,g_player.pos.y,12.0f,260.0f,0.35f,1.0f,0.4f,0.9f); }
        snd_play(SFX_PICKUP);
      } else {
        g_bits++;
        spawn_part(p->pos.x,p->pos.y,0,-60.0f,0.2f,3.0f,1.0f,0.95f,0.3f);
        snd_play(SFX_COIN);
      }
    }
  }

  /* 폭발사망 폭탄 (Volatile, §8.5): 0.5s 후 r90 폭발, 플레이어 2뎀 */
  for(i=0;i<MAXBOOM;i++){ if(!g_boom[i].active) continue; Boom*q=&g_boom[i];
    q->t-=dt;
    if(q->t<=0.0f){ q->active=0;
      float dx=g_player.pos.x-q->pos.x, dy=g_player.pos.y-q->pos.y;
      if(dx*dx+dy*dy<(90.0f+g_player.radius)*(90.0f+g_player.radius)) hurt_player(2.0f);
      burst(q->pos.x,q->pos.y,24,300.0f,1.0f,0.35f,0.15f);
      spawn_ring(q->pos.x,q->pos.y,12.0f,480.0f,0.25f,1.0f,0.4f,0.2f);
      add_trauma(0.3f); snd_play(SFX_DEATH);
    }
  }

  /* 파티클/링/잔상 */
  for(i=0;i<MAXPART;i++){ if(!g_part[i].active) continue; Part*p=&g_part[i];
    p->pos.x+=p->vel.x*dt; p->pos.y+=p->vel.y*dt; p->vel.x*=0.92f; p->vel.y*=0.92f; p->life-=dt; if(p->life<=0.0f) p->active=0; }
  for(i=0;i<MAXRING;i++){ if(!g_ring[i].active) continue; Ring*q=&g_ring[i];
    q->r+=q->vr*dt; q->life-=dt; if(q->life<=0.0f) q->active=0; }
  for(i=0;i<MAXAFTER;i++){ if(!g_after[i].active) continue;
    g_after[i].life-=dt; if(g_after[i].life<=0.0f) g_after[i].active=0; }
  if(g_hurtFx>0.0f){ g_hurtFx-=2.5f*dt; if(g_hurtFx<0.0f)g_hurtFx=0.0f; }
  if(g_fringeFx>0.0f){ g_fringeFx-=3.0f*dt; if(g_fringeFx<0.0f)g_fringeFx=0.0f; }
  if(g_clearFx>0.0f){ g_clearFx-=6.0f*dt; if(g_clearFx<0.0f)g_clearFx=0.0f; }

  /* 전투방 잠금/전멸/개방 */
  int ptx=(int)(g_player.pos.x/TILEF), pty=(int)(g_player.pos.y/TILEF), cgx=ptx/CELLT, cgy=pty/CELLT;
  if(cgx>=0&&cgy>=0&&cgx<GW&&cgy<GH&&g_grid[cgy][cgx].placed) g_seen[cgy][cgx]=1; /* 미니맵 탐험 기록 */
  if(!g_locked && cgx>=0&&cgy>=0&&cgx<GW&&cgy<GH && g_grid[cgy][cgx].placed){
    Cell*c=&g_grid[cgy][cgx];
    if(c->type==1 && !g_cleared[cgy][cgx] && ptx>=c->rx&&ptx<c->rx+c->rw&&pty>=c->ry&&pty<c->ry+c->rh){
      g_locked=1; g_lockGX=cgx; g_lockGY=cgy; spawn_wave(cgx,cgy);
      snd_play(SFX_DOORCLOSE);
    }
    /* 모듈방 제단 (§11): 접근 시 커먼 3택1 */
    if(c->type==2 && !g_cleared[cgy][cgx]){
      float ax=(c->rx+c->rw*0.5f)*TILEF, ay=(c->ry+c->rh*0.5f)*TILEF;
      float adx=g_player.pos.x-ax, ady=g_player.pos.y-ay;
      if(adx*adx+ady*ady<45.0f*45.0f){
        g_cleared[cgy][cgx]=1; draw3(0); g_upgRare=0; g_state=ST_UPG;
        snd_play(SFX_PICKUP);
      }
    }
    /* 보스방 진입 (§9): 보스 레이어의 다운링크방 → 보스 + 잠금 */
    if(boss_layer()&&!g_bossDead&&!g_boss.active&&cgx==g_downGX&&cgy==g_downGY
       &&ptx>=c->rx+1&&ptx<c->rx+c->rw-1&&pty>=c->ry+1&&pty<c->ry+c->rh-1){
      int kk=g_depth/3;
      g_boss.active=1; { int m3=(kk-1)%3; g_boss.type=(unsigned char)((m3==0)?0:(m3==1)?1:2); } /* CORE(d3)/WARDEN(d6)/NEXUS(d9) 순환 */
      g_boss.maxhp=g_boss.hp=400.0f*(1.0f+0.5f*(float)(kk-1));
      g_boss.phase=1; g_boss.flash=0; g_boss.dashState=0;
      g_boss.pos.x=(c->rx+c->rw*0.5f)*TILEF; g_boss.pos.y=(c->ry+c->rh*0.5f)*TILEF;
      g_boss.t1=0; g_boss.t2=0; g_boss.t3=0; g_boss.segAng=0; g_boss.trailT=0; g_boss.stateT=0;
      g_boss.ldir.x=1; g_boss.ldir.y=0; g_boss.wdir.x=1; g_boss.wdir.y=0; g_boss.dashDir.x=1; g_boss.dashDir.y=0;
      g_boss.shrink=0.5f*(float)(c->rw>c->rh?c->rw:c->rh)*TILEF;
      g_locked=1; g_lockGX=cgx; g_lockGY=cgy;
      snd_play(SFX_PHASE); snd_play(SFX_DOORCLOSE); add_trauma(0.3f);
    }
  }
  if(g_locked){ Cell*c=&g_grid[g_lockGY][g_lockGX];
    float mnx=(c->rx+1)*TILEF, mxx=(c->rx+c->rw-1)*TILEF, mny=(c->ry+1)*TILEF, mxy=(c->ry+c->rh-1)*TILEF;
    if(g_player.pos.x<mnx)g_player.pos.x=mnx; if(g_player.pos.x>mxx)g_player.pos.x=mxx;
    if(g_player.pos.y<mny)g_player.pos.y=mny; if(g_player.pos.y>mxy)g_player.pos.y=mxy;
    /* 적도 방 안에 confine */
    for(j=0;j<MAXENE;j++){ if(!g_ene[j].active) continue; float er=ene_radius(g_ene[j].type);
      float emnx=c->rx*TILEF+er, emxx=(c->rx+c->rw)*TILEF-er, emny=c->ry*TILEF+er, emxy=(c->ry+c->rh)*TILEF-er;
      if(g_ene[j].pos.x<emnx){ g_ene[j].pos.x=emnx; if(g_ene[j].type==2) g_ene[j].vel.x=-g_ene[j].vel.x; }
      else if(g_ene[j].pos.x>emxx){ g_ene[j].pos.x=emxx; if(g_ene[j].type==2) g_ene[j].vel.x=-g_ene[j].vel.x; }
      if(g_ene[j].pos.y<emny){ g_ene[j].pos.y=emny; if(g_ene[j].type==2) g_ene[j].vel.y=-g_ene[j].vel.y; }
      else if(g_ene[j].pos.y>emxy){ g_ene[j].pos.y=emxy; if(g_ene[j].type==2) g_ene[j].vel.y=-g_ene[j].vel.y; }
    }
    int alive=0; for(i=0;i<MAXENE;i++) if(g_ene[i].active) alive++;
    if(alive==0&&!g_boss.active){ g_cleared[g_lockGY][g_lockGX]=1; g_locked=0;
      g_clearFx=1.0f; /* 0.1s 정화 플래시 */
      snd_play(SFX_DOOROPEN);
      spawn_ring(g_player.pos.x,g_player.pos.y,12.0f,500.0f,0.4f,0.2f,1.0f,0.9f);
      for(i=0;i<MAXPICK;i++) if(g_pick[i].active) g_pick[i].homing=1; /* 잔여 BITS 자동 수거 */
    }
  }

  /* 다운링크 하강 (잠금 아닐 때만, 보스층은 처치 후) */
  if(!g_locked&&(!boss_layer()||g_bossDead)){
    float dcx=(g_grid[g_downGY][g_downGX].rx+g_grid[g_downGY][g_downGX].rw*0.5f)*TILEF;
    float dcy=(g_grid[g_downGY][g_downGX].ry+g_grid[g_downGY][g_downGX].rh*0.5f)*TILEF;
    float ddx=g_player.pos.x-dcx, ddy=g_player.pos.y-dcy;
    if(ddx*ddx+ddy*ddy<2500.0f) descend();
  }
}

static void camera_update(float rdt){
  vec2 t=g_player.pos;
  float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
  float dx=mwx-g_player.pos.x, dy=mwy-g_player.pos.y, dl=f_sqrt(dx*dx+dy*dy);
  if(dl>1.0f){ float la=dl>60.0f?60.0f:dl; t.x+=dx/dl*la; t.y+=dy/dl*la; }
  float cf=7.2f*rdt; if(cf>1.0f)cf=1.0f; /* 프레임레이트 독립 추적 (det: 60·144Hz 동일 수렴) */
  g_cam.x+=(t.x-g_cam.x)*cf; g_cam.y+=(t.y-g_cam.y)*cf;
  g_kick.x-=g_kick.x*10.0f*rdt; g_kick.y-=g_kick.y*10.0f*rdt;
}

static void circle_line(float cx,float cy,float r,int seg){
  int i; glBegin(GL_LINE_LOOP);
  for(i=0;i<seg;i++){ float a=(float)i/(float)seg*6.2831853f; glVertex2f(cx+f_cos(a)*r,cy+f_sin(a)*r); }
  glEnd();
}
static void circle_fill(float cx,float cy,float r,int seg,float cr,float cg,float cb,float a){
  int i; glColor4f(cr,cg,cb,a);
  glBegin(GL_TRIANGLE_FAN); glVertex2f(cx,cy);
  for(i=0;i<=seg;i++){ float t=(float)i/(float)seg*6.2831853f; glVertex2f(cx+f_cos(t)*r,cy+f_sin(t)*r); }
  glEnd();
}
static void draw_beam(float x,float y,float ux,float uy,float len,float w2,float cr,float cg,float cb,float a){
  float px=-uy, py=ux;
  glColor4f(cr,cg,cb,a);
  glBegin(GL_QUADS);
  glVertex2f(x+px*w2,y+py*w2); glVertex2f(x+ux*len+px*w2,y+uy*len+py*w2);
  glVertex2f(x+ux*len-px*w2,y+uy*len-py*w2); glVertex2f(x-px*w2,y-py*w2);
  glEnd();
}
static void arc_line(float cx,float cy,float r,float frac,int seg){
  int i; if(frac<=0.0f) return; glBegin(GL_LINE_STRIP);
  for(i=0;i<=seg;i++){ float a=-1.5707963f+(float)i/(float)seg*6.2831853f*frac; glVertex2f(cx+f_cos(a)*r,cy+f_sin(a)*r); }
  glEnd();
}
static void diamond(float cx,float cy,float r){
  glBegin(GL_TRIANGLE_FAN); glColor3f(1.0f,1.0f,1.0f); glVertex2f(cx,cy);
    glColor3f(0.0f,0.6f,0.8f); glVertex2f(cx,cy-r); glVertex2f(cx+r,cy); glVertex2f(cx,cy+r); glVertex2f(cx-r,cy); glVertex2f(cx,cy-r);
  glEnd();
}
static void diamond_fill(float cx,float cy,float r,float cr,float cg,float cb,float a){
  glColor4f(cr,cg,cb,a);
  glBegin(GL_QUADS); glVertex2f(cx,cy-r); glVertex2f(cx+r,cy); glVertex2f(cx,cy+r); glVertex2f(cx-r,cy); glEnd();
}
static void diamond_line(float cx,float cy,float r){
  glBegin(GL_LINE_LOOP); glVertex2f(cx,cy-r); glVertex2f(cx+r,cy); glVertex2f(cx,cy+r); glVertex2f(cx-r,cy); glEnd();
}
static void hex_fill(float x,float y,float r){
  int k; glBegin(GL_TRIANGLE_FAN); glVertex2f(x,y);
  for(k=0;k<=6;k++){ float a=(float)k/6.0f*6.2831853f; glVertex2f(x+f_cos(a)*r,y+f_sin(a)*r); }
  glEnd();
}
static void draw_enemy(Enemy*e){
  float r=ene_radius(e->type), x=e->pos.x, y=e->pos.y;
  float fr,fg,fb;
  if(e->type==0){ fr=1.0f; fg=0.30f; fb=0.45f; }
  else if(e->type==1){ fr=0.30f; fg=0.75f; fb=1.0f; }
  else if(e->type==2){ fr=1.0f; fg=0.80f; fb=0.20f; }
  else if(e->type==5){ fr=1.0f; fg=0.45f; fb=0.55f; } /* LANCER = 핫핑크 */
  else if(e->type==6){ fr=0.65f; fg=0.45f; fb=1.0f; } /* WEAVER = 바이올렛 */
  else { fr=0.45f; fg=1.0f; fb=0.40f; } /* 포크/샤드 = 그린 */
  if(e->affix) r*=1.15f; /* 엘리트 약간 큼 */
  if(e->spawn>0.0f){ /* 스폰 텔레그래프: 수축 링 + 마커 */
    float k=e->spawn/0.4f;
    glColor4f(fr,fg,fb,0.7f); circle_line(x,y,r+22.0f*k,16);
    diamond_fill(x,y,r*0.4f,fr,fg,fb,0.5f);
    return;
  }
  int fl=e->flash>0.0f;
  float gpop=fl?0.4f:0.22f;            /* 글로우 알파 (피격 시 강화) */
  float t=g_time;
  float flick=1.0f;                    /* 저HP 점멸 (코어만) */
  if(e->maxhp>0.0f && e->hp<0.30f*e->maxhp) flick=0.55f+0.45f*f_sin(t*14.0f);
  if(e->type==0){ /* HUNTER: 글로우 + 회전 트윈팽 + 맥동 코어 */
    int kk; circle_fill(x,y,r*1.7f,10,fr,fg,fb,gpop);
    float a=t*3.0f; glColor4f(fr,fg,fb,0.6f);
    for(kk=0;kk<2;kk++){ float aa=a+(float)kk*3.14159265f;
      glBegin(GL_TRIANGLES); glVertex2f(x+f_cos(aa)*r*1.35f,y+f_sin(aa)*r*1.35f);
      glVertex2f(x+f_cos(aa+0.35f)*r*0.55f,y+f_sin(aa+0.35f)*r*0.55f);
      glVertex2f(x+f_cos(aa-0.35f)*r*0.55f,y+f_sin(aa-0.35f)*r*0.55f); glEnd(); }
    float cs=r*(1.0f+0.06f*f_sin(t*9.0f));
    if(fl){ fr=fg=fb=1.0f; } glColor3f(fr*flick,fg*flick,fb*flick);
    glBegin(GL_TRIANGLES); glVertex2f(x,y-cs); glVertex2f(x+cs*0.9f,y+cs*0.7f); glVertex2f(x-cs*0.9f,y+cs*0.7f); glEnd();
  }
  else if(e->type==1){ /* TURRET: 글로우 + 회전 볼트프레임 + 코어 + 차지텔 */
    int kk; circle_fill(x,y,r*1.6f,10,fr,fg,fb,gpop);
    float a=t*0.6f; glColor4f(fr,fg,fb,0.45f+0.15f*f_sin(t*4.0f));
    glBegin(GL_LINE_LOOP);
    for(kk=0;kk<4;kk++){ float aa=a+(float)kk*1.5707963f; glVertex2f(x+f_cos(aa)*r*1.25f,y+f_sin(aa)*r*1.25f); } glEnd();
    if(fl){ fr=fg=fb=1.0f; } glColor3f(fr*flick,fg*flick,fb*flick);
    glBegin(GL_QUADS); glVertex2f(x-r,y-r); glVertex2f(x+r,y-r); glVertex2f(x+r,y+r); glVertex2f(x-r,y+r); glEnd();
    if(e->t2>1.3f){ float ch=(e->t2-1.3f)/0.3f; if(ch>1.0f)ch=1.0f;
      glColor4f(1.0f,1.0f,1.0f,ch); glBegin(GL_QUADS); glVertex2f(x-r*0.4f,y-r*0.4f); glVertex2f(x+r*0.4f,y-r*0.4f); glVertex2f(x+r*0.4f,y+r*0.4f); glVertex2f(x-r*0.4f,y+r*0.4f); glEnd(); }
  }
  else if(e->type==2){ /* RICOCHET: 글로우 + 모션트레일 + 회전샤드 + 지터코어 */
    int kk; diamond_fill(x,y,r*1.6f,fr,fg,fb,gpop);
    diamond_fill(x-e->vel.x*0.02f,y-e->vel.y*0.02f,r*0.8f,fr,fg,fb,0.25f);
    float a=t*-5.0f;
    for(kk=0;kk<3;kk++){ float aa=a+(float)kk*2.0943951f; diamond_fill(x+f_cos(aa)*r*1.2f,y+f_sin(aa)*r*1.2f,2.5f,fr,fg,fb,0.55f); }
    float cs=r*(1.0f+0.08f*f_sin(t*12.0f));
    if(fl){ fr=fg=fb=1.0f; } glColor3f(fr*flick,fg*flick,fb*flick);
    glBegin(GL_QUADS); glVertex2f(x,y-cs); glVertex2f(x+cs,y); glVertex2f(x,y+cs); glVertex2f(x-cs,y); glEnd();
  }
  else if(e->type==3){ /* FORK: 글로우 + 회전 헥스프레임 + 분열심 + 코어 */
    int kk; glColor4f(fr,fg,fb,gpop); hex_fill(x,y,r*1.5f);
    float a=t*0.8f;
    glColor4f(fr,fg,fb,0.4f); glBegin(GL_LINE_LOOP);
    for(kk=0;kk<6;kk++){ float aa=(float)kk*1.0471976f+a; glVertex2f(x+f_cos(aa)*r*1.3f,y+f_sin(aa)*r*1.3f); } glEnd();
    glColor4f(fr,fg,fb,0.5f); glBegin(GL_LINES);
    glVertex2f(x-f_cos(a)*r,y-f_sin(a)*r); glVertex2f(x+f_cos(a)*r,y+f_sin(a)*r);
    glVertex2f(x-f_cos(a+1.5707963f)*r,y-f_sin(a+1.5707963f)*r); glVertex2f(x+f_cos(a+1.5707963f)*r,y+f_sin(a+1.5707963f)*r); glEnd();
    if(fl){ fr=fg=fb=1.0f; } glColor3f(fr*flick,fg*flick,fb*flick); hex_fill(x,y,r);
  }
  else if(e->type==5){ /* LANCER: 글로우 + 방향 화살촉 + 윙 + 돌진 텔레그래프 */
    float hx=e->vel.x, hy=e->vel.y, hl=f_sqrt(hx*hx+hy*hy); if(hl<0.01f){ hx=0.0f; hy=-1.0f; hl=1.0f; } hx/=hl; hy/=hl;
    float pxv=-hy, pyv=hx;
    circle_fill(x,y,r*1.5f,8,fr,fg,fb,gpop);
    { float t2=e->t2/((e->affix&2)?0.7f:1.0f);
      if(t2>=0.5f&&t2<1.1f){ float wa=(t2-0.5f)/0.6f; glColor4f(1.0f,0.4f,0.45f,wa*0.7f);
        glBegin(GL_LINES); glVertex2f(x,y); glVertex2f(x+hx*260.0f,y+hy*260.0f); glEnd(); } }
    if(fl){ fr=fg=fb=1.0f; } glColor3f(fr*flick,fg*flick,fb*flick);
    glBegin(GL_TRIANGLES);
    glVertex2f(x+hx*r*1.4f,y+hy*r*1.4f);
    glVertex2f(x-hx*r*0.8f+pxv*r*0.7f,y-hy*r*0.8f+pyv*r*0.7f);
    glVertex2f(x-hx*r*0.8f-pxv*r*0.7f,y-hy*r*0.8f-pyv*r*0.7f); glEnd();
    glColor4f(fr,fg,fb,0.7f); glBegin(GL_LINES);
    glVertex2f(x-hx*r*0.3f,y-hy*r*0.3f); glVertex2f(x-hx*r*1.0f+pxv*r*1.2f,y-hy*r*1.0f+pyv*r*1.2f);
    glVertex2f(x-hx*r*0.3f,y-hy*r*0.3f); glVertex2f(x-hx*r*1.0f-pxv*r*1.2f,y-hy*r*1.0f-pyv*r*1.2f); glEnd();
  }
  else if(e->type==6){ /* WEAVER: 글로우 + 역회전 헥스 2겹 + 코어 */
    int kk; glColor4f(fr,fg,fb,gpop); hex_fill(x,y,r*1.6f);
    float a=t*1.5f;
    if(fl){ fr=fg=fb=1.0f; } glColor4f(fr*flick,fg*flick,fb*flick,0.9f);
    glBegin(GL_LINE_LOOP);
    for(kk=0;kk<6;kk++){ float aa=(float)kk*1.0471976f+a; glVertex2f(x+f_cos(aa)*r,y+f_sin(aa)*r); } glEnd();
    glBegin(GL_LINE_LOOP);
    for(kk=0;kk<6;kk++){ float aa=(float)kk*1.0471976f-a*1.4f; glVertex2f(x+f_cos(aa)*r*0.6f,y+f_sin(aa)*r*0.6f); } glEnd();
    circle_fill(x,y,r*0.35f,10,fr*flick,fg*flick,fb*flick,0.95f);
  }
  else { /* SHARD: 글로우 + 회전 삼각 + 중심점 */
    int kk; circle_fill(x,y,r*1.5f,8,fr,fg,fb,gpop);
    float a=t*4.0f;
    if(fl){ fr=fg=fb=1.0f; } glColor3f(fr*flick,fg*flick,fb*flick);
    glBegin(GL_TRIANGLES);
    for(kk=0;kk<3;kk++){ float aa=a+(float)kk*2.0943951f; glVertex2f(x+f_cos(aa)*r,y+f_sin(aa)*r); } glEnd();
    diamond_fill(x,y,2.0f,1.0f,1.0f,1.0f,0.7f);
  }
  /* 엘리트: 어픽스 배지 (글리프 5종, §8.5) */
  if(e->affix){
    float ar,ag,ab; int bit;
    if(e->affix&1){ ar=0.5f; ag=0.95f; ab=1.0f; bit=1; }
    else if(e->affix&2){ ar=1.0f; ag=0.95f; ab=0.3f; bit=2; }
    else if(e->affix&4){ ar=1.0f; ag=0.4f; ab=0.15f; bit=4; }
    else if(e->affix&8){ ar=0.4f; ag=1.0f; ab=0.5f; bit=8; }
    else { ar=0.8f; ag=0.4f; ab=1.0f; bit=16; }
    glColor4f(ar,ag,ab,0.85f); circle_line(x,y,r+5.0f,12);
    float oa=g_time*4.0f, mx=x+f_cos(oa)*(r+10.0f), my=y+f_sin(oa)*(r+10.0f);
    if(bit==1){ glColor4f(ar,ag,ab,0.9f); circle_line(mx,my,4.0f,8); }
    else if(bit==2){ glColor4f(ar,ag,ab,0.9f); glBegin(GL_LINES); glVertex2f(mx-f_cos(oa)*2.0f,my-f_sin(oa)*2.0f); glVertex2f(mx-f_cos(oa)*8.0f,my-f_sin(oa)*8.0f); glEnd(); }
    else if(bit==4){ float ps=4.0f*(1.0f+0.4f*f_sin(g_time*10.0f)); diamond_fill(mx,my,ps,ar,ag,ab,0.9f); }
    else if(bit==8){ diamond_fill(mx-3.0f,my,3.0f,ar,ag,ab,0.9f); diamond_fill(mx+3.0f,my,3.0f,ar,ag,ab,0.9f); }
    else { glColor4f(ar,ag,ab,0.9f); circle_line(mx,my,5.0f,8); diamond_fill(mx,my,1.5f,ar,ag,ab,0.9f); }
    if(e->eshield){ glColor4f(0.5f,0.95f,1.0f,0.6f); circle_line(x,y,r+9.0f,12); }
  }
}

static void hud_begin(int w,int h){
  glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,w,h,0,-1,1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

static void render_world(int w,int h){
  int tx,ty,i;
  int pt=pal_tier();
  float acR=g_palAc[pt][0], acG=g_palAc[pt][1], acB=g_palAc[pt][2];
  float shk=g_trauma*g_trauma*SHAKE_MAX*g_optShake;
  float sx=fxsym()*shk+g_kick.x, sy=fxsym()*shk+g_kick.y;
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  float L=g_cam.x-w*0.5f+sx, R=g_cam.x+w*0.5f+sx, T=g_cam.y-h*0.5f+sy, B=g_cam.y+h*0.5f+sy;
  glOrtho(L,R,B,T,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glClearColor(g_palBg[pt][0]*0.6f,g_palBg[pt][1]*0.6f,g_palBg[pt][2]*0.6f,1.0f); glClear(GL_COLOR_BUFFER_BIT);

  int tx0=(int)(L/TILEF)-1, tx1=(int)(R/TILEF)+1, ty0=(int)(T/TILEF)-1, ty1=(int)(B/TILEF)+1;
  if(tx0<0)tx0=0; if(ty0<0)ty0=0; if(tx1>MAPW-1)tx1=MAPW-1; if(ty1>MAPH-1)ty1=MAPH-1;

  glBegin(GL_QUADS); glColor3f(g_palBg[pt][0]*1.7f,g_palBg[pt][1]*1.7f,g_palBg[pt][2]*1.7f);
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){ unsigned char t=g_tiles[ty][tx]; if(!(T_WALK(t)||t==4)) continue;
    float X=tx*TILEF, Y=ty*TILEF; glVertex2f(X,Y); glVertex2f(X+TILEF,Y); glVertex2f(X+TILEF,Y+TILEF); glVertex2f(X,Y+TILEF); }
  glEnd();
  /* 바닥 도트 그리드 (§15 기판 느낌) */
  glPointSize(2.0f); glColor4f(acR,acG,acB,0.14f); glBegin(GL_POINTS);
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){ if(!T_WALK(g_tiles[ty][tx])) continue;
    glVertex2f(tx*TILEF+TILEF*0.5f,ty*TILEF+TILEF*0.5f); }
  glEnd();

  glBegin(GL_LINES); glColor3f(acR,acG,acB);
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){ unsigned char t=g_tiles[ty][tx]; if(!(T_WALK(t)||t==4)) continue; float X=tx*TILEF, Y=ty*TILEF;
    if(ty==0||g_tiles[ty-1][tx]==0||g_tiles[ty-1][tx]==3){ glVertex2f(X,Y); glVertex2f(X+TILEF,Y); }
    if(ty>=MAPH-1||g_tiles[ty+1][tx]==0||g_tiles[ty+1][tx]==3){ glVertex2f(X,Y+TILEF); glVertex2f(X+TILEF,Y+TILEF); }
    if(tx==0||g_tiles[ty][tx-1]==0||g_tiles[ty][tx-1]==3){ glVertex2f(X,Y); glVertex2f(X,Y+TILEF); }
    if(tx>=MAPW-1||g_tiles[ty][tx+1]==0||g_tiles[ty][tx+1]==3){ glVertex2f(X+TILEF,Y); glVertex2f(X+TILEF,Y+TILEF); } }
  glEnd();

  /* 크레이트 (앰버 박스 + X, docs/06 §3.4) */
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){ if(g_tiles[ty][tx]!=4) continue;
    float X=tx*TILEF+6.0f, Y=ty*TILEF+6.0f, S=TILEF-12.0f;
    glColor4f(0.55f,0.38f,0.10f,0.55f);
    glBegin(GL_QUADS); glVertex2f(X,Y); glVertex2f(X+S,Y); glVertex2f(X+S,Y+S); glVertex2f(X,Y+S); glEnd();
    glColor4f(1.0f,0.72f,0.25f,0.9f);
    glBegin(GL_LINE_LOOP); glVertex2f(X,Y); glVertex2f(X+S,Y); glVertex2f(X+S,Y+S); glVertex2f(X,Y+S); glEnd();
    glBegin(GL_LINES); glVertex2f(X,Y); glVertex2f(X+S,Y+S); glVertex2f(X+S,Y); glVertex2f(X,Y+S); glEnd();
  }

  /* 잠긴 문 = 네온 배리어 (잠금 방 가장자리 펄스) */
  if(g_locked){ Cell*c=&g_grid[g_lockGY][g_lockGX];
    float pul=0.55f+0.3f*f_sin(g_time*8.0f);
    glColor4f(1.0f,0.25f,0.45f,pul);
    glBegin(GL_LINE_LOOP);
    glVertex2f(c->rx*TILEF,c->ry*TILEF); glVertex2f((c->rx+c->rw)*TILEF,c->ry*TILEF);
    glVertex2f((c->rx+c->rw)*TILEF,(c->ry+c->rh)*TILEF); glVertex2f(c->rx*TILEF,(c->ry+c->rh)*TILEF);
    glEnd();
  }

  /* 다운링크 (펄스) — 보스층은 처치 후에만 출현 */
  if(!boss_layer()||g_bossDead){
    float dcx=(g_grid[g_downGY][g_downGX].rx+g_grid[g_downGY][g_downGX].rw*0.5f)*TILEF, dcy=(g_grid[g_downGY][g_downGX].ry+g_grid[g_downGY][g_downGX].rh*0.5f)*TILEF;
    float dpul=1.0f+0.15f*f_sin(g_time*4.0f);
    glColor3f(0.2f,1.0f,0.85f); diamond_line(dcx,dcy,26.0f*dpul); diamond_line(dcx,dcy,16.0f*dpul);
  }
  { int gx,gy; for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) if(g_grid[gy][gx].placed&&g_grid[gy][gx].type==2){
      float mx=(g_grid[gy][gx].rx+g_grid[gy][gx].rw*0.5f)*TILEF, my=(g_grid[gy][gx].ry+g_grid[gy][gx].rh*0.5f)*TILEF;
      if(!g_cleared[gy][gx]){ /* 미사용 제단: 펄스 */
        float ap=1.0f+0.12f*f_sin(g_time*5.0f);
        glColor3f(1.0f,0.3f,0.85f); diamond_line(mx,my,20.0f*ap); diamond_line(mx,my,11.0f*ap);
        diamond_fill(mx,my,6.0f,1.0f,0.4f,0.9f,0.5f+0.3f*f_sin(g_time*5.0f));
      } else { glColor4f(1.0f,0.3f,0.85f,0.25f); diamond_line(mx,my,20.0f); } } }

  /* 픽업 (BITS / 모듈 오브) */
  for(i=0;i<MAXPICK;i++){ if(!g_pick[i].active) continue; Pickup*p=&g_pick[i];
    float pl=0.7f+0.3f*f_sin(p->t*9.0f);
    if(p->kind==1){
      diamond_fill(p->pos.x,p->pos.y,12.0f,1.0f,0.3f,0.9f,0.3f);
      diamond_fill(p->pos.x,p->pos.y,7.0f,1.0f,0.5f,1.0f,pl);
      glColor4f(1.0f,0.4f,0.95f,0.8f); circle_line(p->pos.x,p->pos.y,14.0f+2.0f*f_sin(p->t*6.0f),12);
    } else {
      diamond_fill(p->pos.x,p->pos.y,7.0f,1.0f,0.95f,0.3f,0.25f);
      diamond_fill(p->pos.x,p->pos.y,4.0f,1.0f,0.95f,0.4f,pl);
    }
  }

  /* 폭발사망 경고 (붉은 펄스 원) */
  for(i=0;i<MAXBOOM;i++){ if(!g_boom[i].active) continue; Boom*q=&g_boom[i];
    float u=1.0f-q->t/(q->t0>0.0f?q->t0:0.5f);
    glColor4f(1.0f,0.3f,0.15f,0.25f+0.55f*u);
    circle_line(q->pos.x,q->pos.y,90.0f,24);
    glColor4f(1.0f,0.3f,0.15f,0.15f+0.3f*u);
    circle_line(q->pos.x,q->pos.y,90.0f*u,16);
  }

  /* 적 발사체 (글로우 2겹 + 펄스) */
  for(i=0;i<MAXEBUL;i++){ if(!g_ebul[i].active) continue; float x=g_ebul[i].pos.x,y=g_ebul[i].pos.y;
    float s=6.0f+1.5f*f_sin(g_time*16.0f+x*0.1f);
    diamond_fill(x,y,s*1.8f,1.0f,0.45f,0.15f,0.3f);
    diamond_fill(x,y,s,1.0f,0.6f,0.25f,1.0f);
  }

  /* 적 */
  for(i=0;i<MAXENE;i++) if(g_ene[i].active) draw_enemy(&g_ene[i]);

  /* 워든 돌진 트레일 */
  for(i=0;i<MAXTRAIL;i++){ if(!g_trail[i].active) continue;
    float a=g_trail[i].life/0.6f;
    diamond_fill(g_trail[i].pos.x,g_trail[i].pos.y,14.0f,1.0f,0.5f,0.15f,a*0.5f); }

  /* 보스 (§9) */
  if(g_boss.active){
    Boss*B=&g_boss; float br=boss_radius(); int ph=B->phase;
    float fr,fg,fb;
    if(B->type==0){ fr=1.0f; fg=0.35f; fb=0.6f; } else if(B->type==2){ fr=0.5f; fg=1.0f; fb=0.8f; } else { fr=1.0f; fg=0.55f; fb=0.2f; }
    if(ph==3){ float fl2=0.5f+0.5f*f_sin(g_time*10.0f); fr=1.0f; fg=0.15f+0.25f*fl2; fb=0.15f; }
    if(B->flash>0.0f){ fr=1.0f; fg=1.0f; fb=1.0f; }
    if(B->type==0){ /* CORE: 페이즈 밴드 맥동 구체 */
      int j; float pr=br*(1.0f+0.05f*f_sin(g_time*(4.0f+(float)ph*2.0f)));
      circle_fill(B->pos.x,B->pos.y,pr*(1.7f+0.15f*(float)ph),28,fr,fg,fb,0.12f+0.04f*(float)ph);
      circle_fill(B->pos.x,B->pos.y,pr*1.25f,28,fr,fg,fb,0.3f);
      circle_fill(B->pos.x,B->pos.y,pr,28,fr,fg,fb,0.9f);
      for(j=0;j<ph;j++){ glColor4f(fr,fg,fb,0.5f-0.1f*(float)j); circle_line(B->pos.x,B->pos.y,pr*(0.55f+0.18f*(float)j),24); } /* 페이즈 동심 링 */
      for(j=0;j<3+ph;j++){ float a=g_time*(1.5f+0.6f*(float)ph)+(float)j*6.2831853f/(float)(3+ph); /* 회전 아이리스 */
        diamond_fill(B->pos.x+f_cos(a)*pr*0.30f,B->pos.y+f_sin(a)*pr*0.30f,4.0f,1.0f,1.0f,1.0f,0.85f); }
      circle_fill(B->pos.x,B->pos.y,pr*0.30f,14,1.0f,1.0f,1.0f,0.95f);
      if(ph>=2&&B->t2<0.8f){ /* 레이저 예고선 + 수렴 아이리스 */
        float cg2=B->t2/0.8f, wa=0.2f+0.5f*cg2;
        draw_beam(B->pos.x,B->pos.y,B->ldir.x,B->ldir.y,700.0f,2.0f,1.0f,0.3f,0.3f,wa);
        if(ph==3) draw_beam(B->pos.x,B->pos.y,-B->ldir.x,-B->ldir.y,700.0f,2.0f,1.0f,0.3f,0.3f,wa);
        glColor4f(1.0f,0.4f,0.4f,0.3f+0.5f*cg2); circle_line(B->pos.x,B->pos.y,br*(0.6f+0.7f*cg2),20);
      }
      if(ph>=2&&B->t2>=0.8f&&B->t2<1.2f){ /* 레이저 발사 + 머즐 팝 */
        draw_beam(B->pos.x,B->pos.y,B->ldir.x,B->ldir.y,700.0f,14.0f,1.0f,0.25f,0.25f,0.35f);
        draw_beam(B->pos.x,B->pos.y,B->ldir.x,B->ldir.y,700.0f,7.0f,1.0f,0.8f,0.8f,0.9f);
        circle_fill(B->pos.x,B->pos.y,br*0.9f,16,1.0f,0.9f,0.9f,0.4f*(1.2f-B->t2)/0.4f);
        if(ph==3){
          draw_beam(B->pos.x,B->pos.y,-B->ldir.x,-B->ldir.y,700.0f,14.0f,1.0f,0.25f,0.25f,0.35f);
          draw_beam(B->pos.x,B->pos.y,-B->ldir.x,-B->ldir.y,700.0f,7.0f,1.0f,0.8f,0.8f,0.9f);
        }
      }
    } else if(B->type==1){ /* WARDEN: 회전 프레임 + 공전 세그먼트 */
      int k; float fa=B->segAng*0.5f;
      glColor4f(fr,fg,fb,0.9f);
      glBegin(GL_LINE_LOOP);
      for(k=0;k<4;k++){ float a=fa+(float)k*1.5707963f; glVertex2f(B->pos.x+f_cos(a)*br,B->pos.y+f_sin(a)*br); }
      glEnd();
      glColor4f(fr,fg,fb,0.5f);
      glBegin(GL_LINE_LOOP);
      for(k=0;k<4;k++){ float a=-fa*1.3f+(float)k*1.5707963f; glVertex2f(B->pos.x+f_cos(a)*br*0.7f,B->pos.y+f_sin(a)*br*0.7f); }
      glEnd();
      { int ng=4+ph; glColor4f(fr,fg,fb,0.6f); glBegin(GL_LINE_LOOP); /* 페이즈 N각 무장링 */
        for(k=0;k<ng;k++){ float a=fa*2.0f+(float)k*6.2831853f/(float)ng; glVertex2f(B->pos.x+f_cos(a)*br*0.55f,B->pos.y+f_sin(a)*br*0.55f); } glEnd(); }
      for(k=0;k<4;k++){ float a=fa+(float)k*1.5707963f; /* 코너 노드 */
        float cs=ph==3?5.0f+2.0f*f_sin(g_time*10.0f):5.0f;
        diamond_fill(B->pos.x+f_cos(a)*br,B->pos.y+f_sin(a)*br,cs,fr,fg,fb,0.8f); }
      if(ph>=2){ glColor4f(1.0f,0.5f,0.2f,0.5f); glBegin(GL_LINES); /* 균열선 (hp 저하) */
        for(k=0;k<ph-1;k++){ float a=-fa+(float)k*2.0943951f;
          glVertex2f(B->pos.x+f_cos(a)*br*0.35f,B->pos.y+f_sin(a)*br*0.35f);
          glVertex2f(B->pos.x+f_cos(a)*br*0.95f,B->pos.y+f_sin(a)*br*0.95f); } glEnd(); }
      circle_fill(B->pos.x,B->pos.y,br*0.35f,16,fr,fg,fb,0.8f);
      circle_fill(B->pos.x,B->pos.y,br*0.18f,12,1.0f,1.0f,1.0f,0.9f);
      int nseg=ph==3?3:2;
      for(k=0;k<nseg;k++){ /* 공전 세그먼트 + 테더 + 선행 트레일 */
        float a=B->segAng+(float)k*6.2831853f/(float)nseg;
        float sgx=B->pos.x+f_cos(a)*130.0f, sgy=B->pos.y+f_sin(a)*130.0f, td=a-0.25f;
        draw_beam(B->pos.x,B->pos.y,f_cos(a),f_sin(a),130.0f,1.5f,fr,fg,fb,0.18f);
        diamond_fill(B->pos.x+f_cos(td)*130.0f,B->pos.y+f_sin(td)*130.0f,8.0f,fr,fg,fb,0.3f);
        diamond_fill(sgx,sgy,22.0f,fr,fg,fb,0.25f);
        diamond_fill(sgx,sgy,15.0f,fr,fg,fb,0.95f);
      }
      if(B->dashState==1){ /* 돌진 조준선 + 차징 코어 펄스 */
        float pc=B->stateT/0.7f, wa=0.25f+0.55f*pc;
        draw_beam(B->pos.x,B->pos.y,B->dashDir.x,B->dashDir.y,700.0f,3.0f,1.0f,0.4f,0.15f,wa);
        circle_fill(B->pos.x,B->pos.y,br*(0.4f+0.5f*pc),16,1.0f,0.5f,0.2f,0.3f+0.4f*pc);
      }
      if(ph>=2){ /* 회전 레이저 암 + 허브 링 */
        int narm=ph==3?3:2;
        glColor4f(1.0f,0.4f,0.15f,0.3f); circle_line(B->pos.x,B->pos.y,br*1.1f,18);
        for(k=0;k<narm;k++){ vec2 ad=B->wdir; vrot(&ad,(float)k*6.2831853f/(float)narm);
          draw_beam(B->pos.x,B->pos.y,ad.x,ad.y,700.0f,14.0f,1.0f,0.3f,0.15f,0.3f);
          draw_beam(B->pos.x,B->pos.y,ad.x,ad.y,700.0f,7.0f,1.0f,0.75f,0.5f,0.85f);
        }
      }
      if(ph==3){ /* 압축 안전 반경 */
        Cell*c2=&g_grid[g_downGY][g_downGX];
        float rcx=(c2->rx+c2->rw*0.5f)*TILEF, rcy=(c2->ry+c2->rh*0.5f)*TILEF;
        glColor4f(1.0f,0.25f,0.15f,0.55f+0.25f*f_sin(g_time*7.0f));
        circle_line(rcx,rcy,B->shrink,32);
        glColor4f(1.0f,0.25f,0.15f,0.2f);
        circle_line(rcx,rcy,B->shrink+8.0f,32);
      }
    } else if(B->type==2){ /* NEXUS: 노드망 + 회전 빔케이지 */
      int kk2; float nd=B->shrink; if(nd>150.0f)nd=150.0f;
      float nx3[3], ny3[3];
      for(kk2=0;kk2<3;kk2++){ float a=B->segAng+(float)kk2*2.0943951f; nx3[kk2]=B->pos.x+f_cos(a)*nd; ny3[kk2]=B->pos.y+f_sin(a)*nd; }
      for(kk2=0;kk2<3;kk2++){ int k3=(kk2+1)%3; /* 케이지 테더 (3변) */
        float ex=nx3[k3]-nx3[kk2], ey=ny3[k3]-ny3[kk2], el=f_sqrt(ex*ex+ey*ey); if(el<0.01f)el=0.01f;
        float live=ph>=2?0.85f:0.5f;
        draw_beam(nx3[kk2],ny3[kk2],ex/el,ey/el,el,ph==3?4.0f:2.5f,fr,fg,fb,live*0.5f);
        draw_beam(nx3[kk2],ny3[kk2],ex/el,ey/el,el,1.0f,1.0f,1.0f,1.0f,live*0.4f); }
      circle_fill(B->pos.x,B->pos.y,br*1.6f,20,fr,fg,fb,0.15f);
      circle_fill(B->pos.x,B->pos.y,br*1.1f,20,fr,fg,fb,0.3f);
      circle_fill(B->pos.x,B->pos.y,br,20,fr,fg,fb,0.85f);
      circle_fill(B->pos.x,B->pos.y,br*0.4f,12,1.0f,1.0f,1.0f,0.95f);
      for(kk2=0;kk2<3;kk2++){ float ps=ph==3?(1.0f+0.25f*f_sin(g_time*10.0f)):1.0f; /* 노드 */
        diamond_fill(nx3[kk2],ny3[kk2],18.0f*ps,fr,fg,fb,0.25f);
        diamond_fill(nx3[kk2],ny3[kk2],11.0f*ps,fr,fg,fb,0.95f);
        diamond_fill(nx3[kk2],ny3[kk2],4.0f,1.0f,1.0f,1.0f,0.9f); }
    }
  }

  /* 플레이어 발사체 = 트레이서(속도 방향 길쭉) + 글로우, 크기=b->r 비례 */
  for(i=0;i<MAXBUL;i++){ if(!g_bul[i].active) continue; Bullet*b=&g_bul[i];
    float vl=f_sqrt(b->vel.x*b->vel.x+b->vel.y*b->vel.y); if(vl<0.001f)vl=1.0f;
    float ux=b->vel.x/vl, uy=b->vel.y/vl, pxv=-uy, pyv=ux;
    float sc=b->r/BUL_R, ln=13.0f*sc, wd=3.0f*sc;
    glColor4f(0.35f,0.9f,1.0f,0.35f); /* 글로우 */
    glBegin(GL_QUADS);
    glVertex2f(b->pos.x-ux*ln*1.6f-pxv*wd*2.2f,b->pos.y-uy*ln*1.6f-pyv*wd*2.2f);
    glVertex2f(b->pos.x+ux*ln*0.6f-pxv*wd*2.2f,b->pos.y+uy*ln*0.6f-pyv*wd*2.2f);
    glVertex2f(b->pos.x+ux*ln*0.6f+pxv*wd*2.2f,b->pos.y+uy*ln*0.6f+pyv*wd*2.2f);
    glVertex2f(b->pos.x-ux*ln*1.6f+pxv*wd*2.2f,b->pos.y-uy*ln*1.6f+pyv*wd*2.2f);
    glEnd();
    glColor3f(0.8f,1.0f,1.0f); /* 코어 */
    glBegin(GL_QUADS);
    glVertex2f(b->pos.x-ux*ln-pxv*wd,b->pos.y-uy*ln-pyv*wd);
    glVertex2f(b->pos.x+ux*4.0f-pxv*wd,b->pos.y+uy*4.0f-pyv*wd);
    glVertex2f(b->pos.x+ux*4.0f+pxv*wd,b->pos.y+uy*4.0f+pyv*wd);
    glVertex2f(b->pos.x-ux*ln+pxv*wd,b->pos.y-uy*ln+pyv*wd);
    glEnd();
  }

  /* 파티클 (가산, 페이드) */
  glBegin(GL_QUADS);
  for(i=0;i<MAXPART;i++){ if(!g_part[i].active) continue; Part*p=&g_part[i]; float a=p->life/p->ilife; if(a<0)a=0;
    glColor4f(p->cr,p->cg,p->cb,a); float x=p->pos.x,y=p->pos.y,s=p->r;
    glVertex2f(x-s,y-s); glVertex2f(x+s,y-s); glVertex2f(x+s,y+s); glVertex2f(x-s,y+s); }
  glEnd();

  /* 링 이펙트 */
  for(i=0;i<MAXRING;i++){ if(!g_ring[i].active) continue; Ring*q=&g_ring[i];
    float a=q->life/q->ilife;
    glColor4f(q->cr,q->cg,q->cb,a); circle_line(q->pos.x,q->pos.y,q->r,20); }

  /* 대시 잔상 */
  for(i=0;i<MAXAFTER;i++){ if(!g_after[i].active) continue;
    float a=g_after[i].life/0.22f*0.5f;
    diamond_fill(g_after[i].pos.x,g_after[i].pos.y,g_player.radius,0.4f,0.95f,1.0f,a); }

  /* 대시 쿨다운 링 (발밑) */
  if(g_dashCd>0.0f){ glColor4f(0.3f,0.9f,1.0f,0.35f); arc_line(g_player.pos.x,g_player.pos.y,g_player.radius+7.0f,1.0f-g_dashCd/dash_cd_max(),18); }

  /* 플레이어 (무적 깜빡임 + 티어 글로우 + 짐벌 링 + 조준 핍 + 수호막, §15) */
  if(!(g_pIfr>0.0f && (((int)(g_pIfr*20.0f))&1))){
    float pr=g_player.radius;
    diamond_fill(g_player.pos.x,g_player.pos.y,pr*1.9f,acR,acG,acB,0.22f);
    if(g_dashT<=0.0f){ int kk; float a=g_time*1.5f; /* 짐벌 트라이프레임 (대시 중엔 잔상이 대체) */
      glColor4f(acR,acG,acB,0.5f); glBegin(GL_LINE_LOOP);
      for(kk=0;kk<3;kk++){ float aa=a+(float)kk*2.0943951f; glVertex2f(g_player.pos.x+f_cos(aa)*(pr+5.0f),g_player.pos.y+f_sin(aa)*(pr+5.0f)); } glEnd(); }
    diamond(g_player.pos.x,g_player.pos.y,pr);
    float mwx=g_cam.x-(float)g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-(float)g_winH*0.5f+(float)g_mouseY;
    float ax=mwx-g_player.pos.x, ay=mwy-g_player.pos.y, al=f_sqrt(ax*ax+ay*ay); if(al<0.001f)al=1.0f;
    diamond_fill(g_player.pos.x+ax/al*(pr+6.0f),g_player.pos.y+ay/al*(pr+6.0f),3.0f,0.8f,1.0f,1.0f,0.9f);
  }
  if(g_shieldUp){ glColor4f(0.4f,0.9f,1.0f,0.5f+0.2f*f_sin(g_time*6.0f));
    circle_line(g_player.pos.x,g_player.pos.y,g_player.radius+10.0f,16); }
}

static void render_hud(int w,int h){
  int i;
  hud_begin(w,h);
  /* 하트 (1하트=2HP, 절반=1HP) */
  for(i=0;i<3;i++){
    float bx=20.0f+i*30.0f, by=24.0f; int hp2=(int)g_pHP-i*2;
    float cr=1.0f,cg=0.25f,cb=0.4f;
    if(hp2>=2) diamond_fill(bx+9.0f,by,11.0f,cr,cg,cb,1.0f);
    else if(hp2==1){ glColor4f(cr,cg,cb,1.0f); glBegin(GL_TRIANGLES); glVertex2f(bx+9.0f,by-11.0f); glVertex2f(bx+9.0f,by+11.0f); glVertex2f(bx-2.0f,by); glEnd();
      glColor4f(0.25f,0.12f,0.16f,1.0f); glBegin(GL_TRIANGLES); glVertex2f(bx+9.0f,by-11.0f); glVertex2f(bx+20.0f,by); glVertex2f(bx+9.0f,by+11.0f); glEnd(); }
    else diamond_fill(bx+9.0f,by,11.0f,0.25f,0.12f,0.16f,1.0f);
  }
  draw_text(20.0f,44.0f,2.0f,"LAYER",0.4f,0.9f,1.0f,0.9f);
  draw_int(20.0f+text_w("LAYER ",2.0f)+6.0f,44.0f,2.0f,g_depth,1.0f,1.0f,1.0f,1.0f);
  /* 보스 HP 바 (상단 중앙) */
  if(g_boss.active){
    float bw=380.0f, bx=((float)w-bw)*0.5f, by=22.0f, fr2=g_boss.hp/g_boss.maxhp;
    if(fr2<0.0f)fr2=0.0f;
    center_text(w,8.0f,1.4f,g_boss.type==0?"SUBCORE - THE CORE":(g_boss.type==2?"SUBCORE - THE NEXUS":"SUBCORE - THE WARDEN"),1.0f,0.4f,0.5f,0.9f);
    glColor4f(0.15f,0.05f,0.1f,0.8f);
    glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+bw,by); glVertex2f(bx+bw,by+10.0f); glVertex2f(bx,by+10.0f); glEnd();
    glColor4f(1.0f,0.3f,0.45f,0.95f);
    glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+bw*fr2,by); glVertex2f(bx+bw*fr2,by+10.0f); glVertex2f(bx,by+10.0f); glEnd();
    glColor4f(1.0f,0.5f,0.6f,0.7f);
    glBegin(GL_LINE_LOOP); glVertex2f(bx,by); glVertex2f(bx+bw,by); glVertex2f(bx+bw,by+10.0f); glVertex2f(bx,by+10.0f); glEnd();
  }
  /* 우상: SCORE / BITS */
  { char buf[12]; fmt_int(buf,score_now());
    float tw=text_w(buf,2.0f);
    draw_text((float)w-tw-text_w("SCORE ",2.0f)-20.0f,18.0f,2.0f,"SCORE",0.4f,0.9f,1.0f,0.9f);
    draw_text((float)w-tw-20.0f,18.0f,2.0f,buf,1.0f,1.0f,1.0f,1.0f);
    fmt_int(buf,g_bits); tw=text_w(buf,2.0f);
    draw_text((float)w-tw-text_w("BITS ",2.0f)-20.0f,44.0f,2.0f,"BITS",1.0f,0.95f,0.3f,0.9f);
    draw_text((float)w-tw-20.0f,44.0f,2.0f,buf,1.0f,0.95f,0.5f,1.0f);
  }
  /* 스킬 게이지 좌하단 (Q EMP / RMB 블링크, docs/06 §2.3) */
  { float gx=20.0f, gy=(float)h-56.0f, gs=36.0f; int k;
    for(k=0;k<2;k++){
      float cd=k==0?g_empCd:g_blinkCd, mx=k==0?EMP_CD:BLINK_CD;
      float f=cd>0.0f?1.0f-cd/mx:1.0f, bx=gx+k*48.0f;
      int ready=cd<=0.0f;
      float pul=ready?0.75f+0.25f*f_sin(g_time*6.0f):0.35f;
      glColor4f(0.1f,0.2f,0.3f,0.6f);
      glBegin(GL_QUADS); glVertex2f(bx,gy); glVertex2f(bx+gs,gy); glVertex2f(bx+gs,gy+gs); glVertex2f(bx,gy+gs); glEnd();
      glColor4f(0.25f,0.8f,1.0f,ready?0.5f:0.3f);
      glBegin(GL_QUADS); glVertex2f(bx,gy+gs*(1.0f-f)); glVertex2f(bx+gs,gy+gs*(1.0f-f)); glVertex2f(bx+gs,gy+gs); glVertex2f(bx,gy+gs); glEnd();
      glColor4f(0.4f,0.9f,1.0f,pul);
      glBegin(GL_LINE_LOOP); glVertex2f(bx,gy); glVertex2f(bx+gs,gy); glVertex2f(bx+gs,gy+gs); glVertex2f(bx,gy+gs); glEnd();
      draw_text(bx+gs*0.5f-4.5f,gy+gs*0.5f-7.5f,3.0f,k==0?"Q":"B",1.0f,1.0f,1.0f,pul+0.2f);
    }
  }
  /* 미니맵 (§18 M6): 우상단, 셀 8px */
  { int gx,gy, pt=pal_tier();
    int pgx=(int)(g_player.pos.x/TILEF)/CELLT, pgy=(int)(g_player.pos.y/TILEF)/CELLT;
    float mx0=(float)w-20.0f-9.0f*10.0f, my0=72.0f;
    for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++){
      if(!g_grid[gy][gx].placed) continue;
      int seen=g_seen[gy][gx];
      int adj=0;
      if(!seen){ /* 인접 미탐험: 외곽선만 */
        if(gy>0&&g_seen[gy-1][gx]&&(g_conn[gy-1][gx]&4)) adj=1;
        if(gy<GH-1&&g_seen[gy+1][gx]&&(g_conn[gy+1][gx]&1)) adj=1;
        if(gx>0&&g_seen[gy][gx-1]&&(g_conn[gy][gx-1]&2)) adj=1;
        if(gx<GW-1&&g_seen[gy][gx+1]&&(g_conn[gy][gx+1]&8)) adj=1;
        if(!adj) continue;
      }
      float bx=mx0+(float)gx*10.0f, by=my0+(float)gy*10.0f;
      if(seen){
        int cur=(gx==pgx&&gy==pgy);
        if(cur) glColor4f(1.0f,1.0f,1.0f,0.95f);
        else glColor4f(g_palAc[pt][0],g_palAc[pt][1],g_palAc[pt][2],0.4f);
        glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+8.0f,by); glVertex2f(bx+8.0f,by+8.0f); glVertex2f(bx,by+8.0f); glEnd();
      } else {
        glColor4f(g_palAc[pt][0],g_palAc[pt][1],g_palAc[pt][2],0.35f);
        glBegin(GL_LINE_LOOP); glVertex2f(bx,by); glVertex2f(bx+8.0f,by); glVertex2f(bx+8.0f,by+8.0f); glVertex2f(bx,by+8.0f); glEnd();
      }
      if(seen){ /* 특수 마커 */
        unsigned char rt=g_grid[gy][gx].type;
        if(rt==2&&!g_cleared[gy][gx]) diamond_fill(bx+4.0f,by+4.0f,3.0f,1.0f,0.3f,0.85f,0.95f);
        else if(rt==3){
          if(boss_layer()&&!g_bossDead){ glColor4f(1.0f,0.3f,0.3f,0.95f);
            glBegin(GL_LINES); glVertex2f(bx+1.5f,by+1.5f); glVertex2f(bx+6.5f,by+6.5f); glVertex2f(bx+6.5f,by+1.5f); glVertex2f(bx+1.5f,by+6.5f); glEnd(); }
          else { glColor4f(0.2f,1.0f,0.85f,0.95f);
            glBegin(GL_TRIANGLES); glVertex2f(bx+1.5f,by+2.0f); glVertex2f(bx+6.5f,by+2.0f); glVertex2f(bx+4.0f,by+6.5f); glEnd(); }
        }
      }
    }
  }
  /* 피격 적색 비네트 (화면 가장자리) */
  if(g_hurtFx>0.0f&&g_optFlash>0.0f){ float a=g_hurtFx*0.4f*g_optFlash; float bw=(float)w*0.18f, bh=(float)h*0.22f;
    glBegin(GL_QUADS);
    glColor4f(1.0f,0.1f,0.15f,a); glVertex2f(0,0); glVertex2f((float)w,0);
    glColor4f(1.0f,0.1f,0.15f,0.0f); glVertex2f((float)w,bh); glVertex2f(0,bh);
    glColor4f(1.0f,0.1f,0.15f,0.0f); glVertex2f(0,(float)h-bh); glVertex2f((float)w,(float)h-bh);
    glColor4f(1.0f,0.1f,0.15f,a); glVertex2f((float)w,(float)h); glVertex2f(0,(float)h);
    glColor4f(1.0f,0.1f,0.15f,a); glVertex2f(0,0); glColor4f(1.0f,0.1f,0.15f,0.0f); glVertex2f(bw,0);
    glVertex2f(bw,(float)h); glColor4f(1.0f,0.1f,0.15f,a); glVertex2f(0,(float)h);
    glColor4f(1.0f,0.1f,0.15f,0.0f); glVertex2f((float)w-bw,0); glColor4f(1.0f,0.1f,0.15f,a); glVertex2f((float)w,0);
    glVertex2f((float)w,(float)h); glColor4f(1.0f,0.1f,0.15f,0.0f); glVertex2f((float)w-bw,(float)h);
    glEnd();
  }
  /* 방 클리어 정화 플래시 */
  if(g_clearFx>0.0f&&g_optFlash>0.0f){ glColor4f(0.5f,1.0f,0.95f,g_clearFx*0.22f*g_optFlash);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f((float)w,0); glVertex2f((float)w,(float)h); glVertex2f(0,(float)h); glEnd(); }
  /* 커스텀 네온 십자선 (스크린 좌표 — 셰이크 비적용) */
  { float mx=(float)g_mouseX, my=(float)g_mouseY;
    glColor3f(1.0f,0.3f,0.6f); glBegin(GL_LINES);
    glVertex2f(mx-9.0f,my); glVertex2f(mx-3.0f,my); glVertex2f(mx+3.0f,my); glVertex2f(mx+9.0f,my);
    glVertex2f(mx,my-9.0f); glVertex2f(mx,my-3.0f); glVertex2f(mx,my+3.0f); glVertex2f(mx,my+9.0f); glEnd();
  }
}

static void dim_screen(int w,int h,float a){
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(0.0f,0.0f,0.02f,a);
  glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f((float)w,0); glVertex2f((float)w,(float)h); glVertex2f(0,(float)h); glEnd();
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
}

/* 셰이더리스 포스트FX: 비네트+CRT코너 / 스캔라인 / 색수차 프린지 / 액션 헤이즈. 광과민=g_optFlash·g_optCRT 게이트 */
static void render_post(int w,int h){
  if(g_optCRT<=0.0f) return;
  hud_begin(w,h);
  float fw=(float)w, fh=(float)h, bw=fw*0.16f, bh=fh*0.16f;
  float dim=g_optCRT*(0.5f+0.5f*g_optFlash);
  float va=0.42f*dim;
  /* (1) 비네트 (에지 그라디언트 4쿼드, 모서리 중첩=CRT 코너 어둠) */
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  glColor4f(0,0,0.012f,va); glVertex2f(0,0); glVertex2f(fw,0); glColor4f(0,0,0.012f,0); glVertex2f(fw,bh); glVertex2f(0,bh);
  glColor4f(0,0,0.012f,0); glVertex2f(0,fh-bh); glVertex2f(fw,fh-bh); glColor4f(0,0,0.012f,va); glVertex2f(fw,fh); glVertex2f(0,fh);
  glColor4f(0,0,0.012f,va); glVertex2f(0,0); glColor4f(0,0,0.012f,0); glVertex2f(bw,0); glVertex2f(bw,fh); glColor4f(0,0,0.012f,va); glVertex2f(0,fh);
  glColor4f(0,0,0.012f,0); glVertex2f(fw-bw,0); glColor4f(0,0,0.012f,va); glVertex2f(fw,0); glVertex2f(fw,fh); glColor4f(0,0,0.012f,0); glVertex2f(fw-bw,fh);
  glEnd();
  /* (2) 스캔라인 (3px 간격, 정적) */
  { float sa=0.13f*dim; int yy; glColor4f(0,0,0,sa); glBegin(GL_QUADS);
    for(yy=0;yy<h;yy+=3){ glVertex2f(0,(float)yy); glVertex2f(fw,(float)yy); glVertex2f(fw,(float)yy+1.0f); glVertex2f(0,(float)yy+1.0f); } glEnd(); }
  /* (3) 색수차 프린지 (히트/보스페이즈, 광과민 시 g_optFlash=0이면 완전 제거) */
  { float fx=g_hurtFx+g_fringeFx; if(fx>1.0f)fx=1.0f; fx*=g_optFlash*g_optCRT; /* 보스 상시 ambient 제거 — 히트/페이즈에만 */
    if(fx>0.01f){ float off=4.0f*fx;
      glBlendFunc(GL_SRC_ALPHA,GL_ONE);
      glColor4f(0.6f,0.05f,0.1f,0.35f*fx); glBegin(GL_LINE_LOOP);
      glVertex2f(3.0f+off,3.0f+off); glVertex2f(fw-3.0f+off,3.0f+off); glVertex2f(fw-3.0f+off,fh-3.0f+off); glVertex2f(3.0f+off,fh-3.0f+off); glEnd();
      glColor4f(0.05f,0.3f,0.6f,0.35f*fx); glBegin(GL_LINE_LOOP);
      glVertex2f(3.0f-off,3.0f-off); glVertex2f(fw-3.0f-off,3.0f-off); glVertex2f(fw-3.0f-off,fh-3.0f-off); glVertex2f(3.0f-off,fh-3.0f-off); glEnd(); } }
  /* (4) 액션 헤이즈 리프트 (no-FBO 의사 블룸, 팰릿 액센트 가산) */
  { int pt=pal_tier(); float ha=(0.015f+(0.05f*g_trauma+0.04f*g_clearFx)*g_optFlash+(g_boss.active?0.01f:0.0f))*g_optCRT;
    if(ha>0.10f)ha=0.10f;
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glColor4f(g_palAc[pt][0],g_palAc[pt][1],g_palAc[pt][2],ha);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(fw,0); glVertex2f(fw,fh); glVertex2f(0,fh); glEnd(); }
  glBlendFunc(GL_SRC_ALPHA,GL_ONE); /* 엔진 기본 가산 블렌드 복원 */
}

static void render_title(int w,int h){
  int i;
  glViewport(0,0,w,h);
  glClearColor(0.02f,0.03f,0.05f,1.0f); glClear(GL_COLOR_BUFFER_BIT);
  hud_begin(w,h);
  /* 배경 부유 파티클 */
  for(i=0;i<MAXPART;i++){ if(!g_part[i].active) continue; Part*p=&g_part[i]; float a=p->life/p->ilife*0.5f;
    glColor4f(p->cr,p->cg,p->cb,a);
    glBegin(GL_QUADS); glVertex2f(p->pos.x-p->r,p->pos.y-p->r); glVertex2f(p->pos.x+p->r,p->pos.y-p->r); glVertex2f(p->pos.x+p->r,p->pos.y+p->r); glVertex2f(p->pos.x-p->r,p->pos.y+p->r); glEnd(); }
  float cy=(float)h*0.30f;
  float pul=0.85f+0.15f*f_sin(g_time*2.4f);
  center_text(w,cy-14.0f,8.2f,"NEON DESCENT",0.0f,0.9f,1.0f,0.25f); /* 글로우 */
  center_text(w,cy-16.0f,8.0f,"NEON DESCENT",0.55f,1.0f,1.0f,pul);
  center_text(w,cy+60.0f,2.4f,"PURGE THE MACHINE",1.0f,0.35f,0.7f,0.8f);
  float bl=0.5f+0.5f*f_sin(g_time*3.2f);
  center_text(w,(float)h*0.62f,2.6f,"CLICK / SPACE - DESCEND",1.0f,1.0f,1.0f,0.35f+0.5f*bl);
  center_text(w,(float)h*0.70f,1.5f,"WASD MOVE - LMB SHOOT - SPACE DASH - RMB BLINK - Q EMP",0.5f,0.85f,1.0f,0.55f);
  if(g_bestScore>0){ char buf[12];
    float y=(float)h*0.78f;
    center_text(w,y,2.0f,"BEST",0.4f,0.9f,1.0f,0.7f);
    fmt_int(buf,g_bestLayer);
    { char line[24]; int n=0; const char*s1="LAYER "; while(*s1)line[n++]=*s1++; const char*p=buf; while(*p)line[n++]=*p++; line[n]=0;
      center_text(w,y+26.0f,2.0f,line,1.0f,1.0f,1.0f,0.8f); }
    fmt_int(buf,g_bestScore);
    { char line[24]; int n=0; const char*s2="SCORE "; while(*s2)line[n++]=*s2++; const char*p=buf; while(*p)line[n++]=*p++; line[n]=0;
      center_text(w,y+52.0f,2.0f,line,1.0f,1.0f,1.0f,0.8f); }
  }
}

static void render_gameover(int w,int h){
  render_world(w,h);
  hud_begin(w,h);
  dim_screen(w,h,0.68f);
  float cy=(float)h*0.22f;
  center_text(w,cy,5.0f,"SIGNAL LOST",1.0f,0.3f,0.4f,0.95f);
  { char buf[12]; char line[28]; int n=0; const char*s="LAYER "; while(*s)line[n++]=*s++;
    fmt_int(buf,g_depth); { const char*p=buf; while(*p)line[n++]=*p++; }
    { const char*s2=" REACHED"; while(*s2)line[n++]=*s2++; } line[n]=0;
    center_text(w,cy+58.0f,2.4f,line,1.0f,1.0f,1.0f,0.85f); }
  /* 점수 분해 (§17) */
  { float y=cy+120.0f; char buf[12]; char line[32]; int n;
    n=0; { const char*s="LAYER X1000 : "; while(*s)line[n++]=*s++; } fmt_int(buf,g_depth*1000); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
    center_text(w,y,2.0f,line,0.6f,0.95f,1.0f,0.85f);
    n=0; { const char*s="KILLS X25 : "; while(*s)line[n++]=*s++; } fmt_int(buf,g_kills*25); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
    center_text(w,y+26.0f,2.0f,line,0.6f,0.95f,1.0f,0.85f);
    n=0; { const char*s="BITS X5 : "; while(*s)line[n++]=*s++; } fmt_int(buf,g_bits*5); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
    center_text(w,y+52.0f,2.0f,line,1.0f,0.95f,0.5f,0.85f);
    n=0; { const char*s="BOSS X500 : "; while(*s)line[n++]=*s++; } fmt_int(buf,g_bossKills*500); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
    center_text(w,y+78.0f,2.0f,line,1.0f,0.5f,0.8f,0.85f);
    n=0; { const char*s="SCORE "; while(*s)line[n++]=*s++; } fmt_int(buf,score_now()); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
    center_text(w,y+122.0f,3.2f,line,1.0f,1.0f,1.0f,1.0f);
    n=0; { const char*s="( BEST "; while(*s)line[n++]=*s++; } fmt_int(buf,g_bestScore); { const char*p=buf; while(*p)line[n++]=*p++; } { const char*s2=" )"; while(*s2)line[n++]=*s2++; } line[n]=0;
    center_text(w,y+158.0f,1.8f,line,0.6f,0.9f,1.0f,0.75f); /* §18 목업 BEST 줄 */
  }
  float bl=0.5f+0.5f*f_sin(g_time*3.2f);
  center_text(w,(float)h*0.82f,2.4f,"R - DESCEND AGAIN",1.0f,1.0f,1.0f,0.35f+0.5f*bl);
  center_text(w,(float)h*0.82f+30.0f,1.6f,"ESC - TITLE",0.7f,0.7f,0.8f,0.6f);
}

static void render_pause(int w,int h){
  render_world(w,h);
  render_hud(w,h);
  hud_begin(w,h);
  dim_screen(w,h,0.55f);
  center_text(w,(float)h*0.38f,5.0f,"PAUSED",0.55f,1.0f,1.0f,0.95f);
  center_text(w,(float)h*0.55f,2.0f,"ESC RESUME - Q QUIT",1.0f,1.0f,1.0f,0.6f);
  /* 옵션 (§18 광과민 대응): S 셰이크 / F 플래시 / M 음소거 */
  { char line[56]; int n=0; const char*s1="S SHAKE "; while(*s1)line[n++]=*s1++;
    { char b2[8]; fmt_int(b2,(int)(g_optShake*100.0f)); const char*p=b2; while(*p)line[n++]=*p++; }
    { const char*s2="  F FLASH "; while(*s2)line[n++]=*s2++; }
    { char b2[8]; fmt_int(b2,(int)(g_optFlash*100.0f)); const char*p=b2; while(*p)line[n++]=*p++; }
    { const char*s3="  C CRT "; while(*s3)line[n++]=*s3++; }
    { char b2[8]; fmt_int(b2,(int)(g_optCRT*100.0f)); const char*p=b2; while(*p)line[n++]=*p++; }
    { const char*s5="  M MUTE "; while(*s5)line[n++]=*s5++; }
    { const char*s4=g_mute?"ON":"OFF"; while(*s4)line[n++]=*s4++; }
    line[n]=0;
    center_text(w,(float)h*0.63f,1.6f,line,0.6f,0.9f,1.0f,0.6f);
  }
}

static void render_upgrade(int w,int h){
  int k;
  render_world(w,h);
  hud_begin(w,h);
  dim_screen(w,h,0.6f);
  center_text(w,(float)h*0.18f,3.4f,g_upgRare?"- RARE MODULE -":"- INSTALL MODULE -",1.0f,0.4f,0.9f,0.95f);
  float cw=240.0f, ch=150.0f, gap=30.0f;
  float x0=((float)w-(cw*3.0f+gap*2.0f))*0.5f, y0=(float)h*0.34f;
  for(k=0;k<3;k++){
    int m=g_upgSel[k];
    float x=x0+(float)k*(cw+gap);
    int hov=(float)g_mouseX>=x&&(float)g_mouseX<=x+cw&&(float)g_mouseY>=y0&&(float)g_mouseY<=y0+ch;
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f,0.07f,0.12f,0.92f);
    glBegin(GL_QUADS); glVertex2f(x,y0); glVertex2f(x+cw,y0); glVertex2f(x+cw,y0+ch); glVertex2f(x,y0+ch); glEnd();
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    if(g_upgRare) glColor4f(1.0f,0.75f,0.25f,hov?1.0f:0.55f);
    else glColor4f(0.3f,0.9f,1.0f,hov?1.0f:0.5f);
    glBegin(GL_LINE_LOOP); glVertex2f(x,y0); glVertex2f(x+cw,y0); glVertex2f(x+cw,y0+ch); glVertex2f(x,y0+ch); glEnd();
    if(m>=0){
      char num[2]; num[0]=(char)('1'+k); num[1]=0;
      draw_text(x+10.0f,y0+10.0f,2.0f,num,0.6f,0.95f,1.0f,0.8f);
      draw_text(x+(cw-text_w(g_modName[m],2.0f))*0.5f,y0+52.0f,2.0f,g_modName[m],1.0f,1.0f,1.0f,1.0f);
      draw_text(x+(cw-text_w(g_modDesc[m],1.4f))*0.5f,y0+92.0f,1.4f,g_modDesc[m],0.6f,0.95f,1.0f,0.85f);
      if(g_mod[m]>0){ char sb[8]; sb[0]='L'; sb[1]='V'; sb[2]=' '; fmt_int(sb+3,g_mod[m]+1);
        draw_text(x+(cw-text_w(sb,1.4f))*0.5f,y0+120.0f,1.4f,sb,1.0f,0.75f,0.3f,0.8f); }
    }
  }
  center_text(w,y0+ch+44.0f,1.8f,"CLICK OR 1 - 2 - 3",1.0f,1.0f,1.0f,0.5f);
}

void WinMainCRTStartup(void){
  HINSTANCE inst=GetModuleHandleA(0);
  static WNDCLASSA wc;
  wc.style=CS_OWNDC; wc.lpfnWndProc=WndProc; wc.hInstance=inst;
  wc.hCursor=LoadCursorA(0,(LPCSTR)IDC_ARROW); wc.lpszClassName="ND";
  RegisterClassA(&wc);
  HWND hwnd=CreateWindowExA(0,"ND","NEON DESCENT",WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,1280,720,0,0,inst,0);
  HDC dc=GetDC(hwnd);
  SetPixelFormat(dc,ChoosePixelFormat(dc,&pfd),&pfd);
  wglMakeCurrent(dc,wglCreateContext(dc));
  ShowCursor(FALSE);

  LARGE_INTEGER li; QueryPerformanceCounter(&li); g_master=(unsigned int)li.QuadPart | 1u; g_rng=g_master; (void)xrnd(); g_rngFx=g_master^0x9E3779B9u; if(!g_rngFx)g_rngFx=0x12345678u;
  g_player.radius=14.0f; g_depth=1;
  snd_init();

  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);

  LARGE_INTEGER freq, prev, now; QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&prev);
  const float STEP=1.0f/120.0f; float acc=0.0f; MSG msg;
  while(!g_quit){
    while(PeekMessageA(&msg,0,0,0,PM_REMOVE)){ if(msg.message==WM_QUIT) g_quit=1; TranslateMessage(&msg); DispatchMessageA(&msg); }
    QueryPerformanceCounter(&now);
    float dt=(float)((double)(now.QuadPart-prev.QuadPart)/(double)freq.QuadPart); prev=now; if(dt>0.25f) dt=0.25f;

    /* 상태 전이 (§17) */
    if(g_state==ST_TITLE){
      if(g_mousePressed||g_kpress[VK_SPACE]||g_kpress[VK_RETURN]){ g_kpress[VK_SPACE]=0; g_kpress[VK_RETURN]=0; new_run(); }
      g_time+=dt;
      /* 타이틀 배경 파티클 (fx — 결정론 무관) */
      if((g_rngFx&7)<3) spawn_part((fxsym()*0.5f+0.5f)*(float)g_winW,(float)g_winH+10.0f,fxsym()*12.0f,-30.0f-(fxsym()*0.5f+0.5f)*40.0f,3.0f,1.5f+(fxsym()*0.5f+0.5f)*2.0f,0.0f,0.8f,1.0f);
      { int i; float pdt=dt; for(i=0;i<MAXPART;i++){ if(!g_part[i].active) continue;
          g_part[i].pos.x+=g_part[i].vel.x*pdt; g_part[i].pos.y+=g_part[i].vel.y*pdt; g_part[i].life-=pdt; if(g_part[i].life<=0.0f)g_part[i].active=0; } }
      acc=0.0f;
    } else if(g_state==ST_PLAY){
      if(g_kpress[VK_ESCAPE]){ g_kpress[VK_ESCAPE]=0; g_state=ST_PAUSE; }
      if(g_kpress['M']){ g_kpress['M']=0; g_mute=!g_mute; }
      if(g_kpress[VK_SPACE]){ g_kpress[VK_SPACE]=0; g_wantDash=1; } /* 에지 → 래치 */
      if(g_kpress['Q']){ g_kpress['Q']=0; g_wantEmp=1; }
      acc+=dt;
      while(acc>=STEP){
        if(g_hitstop>0.0f){ g_hitstop-=STEP; }
        else if(g_slowT>0.0f){ /* 시간왜곡 슬로모 0.3x (결정론적 브레젠험 스텝) */
          g_slowT-=STEP; g_slowAcc+=0.3f;
          if(g_slowAcc>=1.0f){ g_slowAcc-=1.0f; player_update(STEP); combat_update(STEP); consume_latches(); }
        }
        else { player_update(STEP); combat_update(STEP); consume_latches(); }
        acc-=STEP;
        if(g_state!=ST_PLAY) { acc=0.0f; break; } /* 사망/모듈선택 등 상태 전이 */
      }
    } else if(g_state==ST_UPG){
      int pick=-1;
      if(g_kpress['1'])pick=0; if(g_kpress['2'])pick=1; if(g_kpress['3'])pick=2;
      if(g_mousePressed){
        float cw=240.0f, ch=150.0f, gap=30.0f;
        float x0=((float)g_winW-(cw*3.0f+gap*2.0f))*0.5f, y0=(float)g_winH*0.34f;
        int k; for(k=0;k<3;k++){ float x=x0+(float)k*(cw+gap);
          if((float)g_mouseX>=x&&(float)g_mouseX<=x+cw&&(float)g_mouseY>=y0&&(float)g_mouseY<=y0+ch) pick=k; }
      }
      if(pick>=0&&g_upgSel[pick]>=0) apply_mod(g_upgSel[pick]);
      g_time+=dt; acc=0.0f;
    } else if(g_state==ST_PAUSE){
      if(g_kpress[VK_ESCAPE]){ g_kpress[VK_ESCAPE]=0; g_state=ST_PLAY; }
      if(g_kpress['Q']){ g_kpress['Q']=0; g_state=ST_TITLE; memset(g_part,0,sizeof(g_part)); g_time=0; }
      if(g_kpress['S']){ g_kpress['S']=0; g_optShake-=0.5f; if(g_optShake<0.0f)g_optShake=1.0f; }
      if(g_kpress['F']){ g_kpress['F']=0; g_optFlash-=0.5f; if(g_optFlash<0.0f)g_optFlash=1.0f; }
      if(g_kpress['C']){ g_kpress['C']=0; g_optCRT-=0.5f; if(g_optCRT<0.0f)g_optCRT=1.0f; }
      if(g_kpress['M']){ g_kpress['M']=0; g_mute=!g_mute; }
      acc=0.0f;
    } else { /* GAMEOVER */
      if(g_kpress['R']){ g_kpress['R']=0; new_run(); }
      if(g_kpress[VK_ESCAPE]){ g_kpress[VK_ESCAPE]=0; g_state=ST_TITLE; memset(g_part,0,sizeof(g_part)); g_time=0; }
      g_time+=dt;
      acc=0.0f;
    }
    memset(g_kpress,0,sizeof(g_kpress)); g_mousePressed=0;
    if(g_state!=ST_PLAY) g_rmbPressed=0;

    RECT rcl; GetClientRect(hwnd,&rcl);
    int w=rcl.right,h=rcl.bottom; if(w<1)w=1; if(h<1)h=1;
    g_winW=w; g_winH=h;
    if(g_state==ST_TITLE){ render_title(w,h); }
    else if(g_state==ST_PLAY){ camera_update(dt); render_world(w,h); render_hud(w,h); render_post(w,h); }
    else if(g_state==ST_UPG){ render_upgrade(w,h); render_post(w,h); }
    else if(g_state==ST_PAUSE){ render_pause(w,h); render_post(w,h); }
    else { render_gameover(w,h); render_post(w,h); }
    SwapBuffers(dc);
    snd_update();
  }
  if(g_sndOn){ int si; waveOutReset(g_wo);
    for(si=0;si<SND_BUFS;si++) waveOutUnprepareHeader(g_wo,&g_whdr[si],sizeof(WAVEHDR));
    waveOutClose(g_wo); }
  ExitProcess(0);
}
