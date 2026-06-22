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
static float bgm_sample(void); /* 절차 BGM (§16.4) — 정의는 게임 상태 전역 이후 */
static void snd_fill(short *buf){
  int s,i;
  for(s=0;s<SND_SAMP;s++){
    float acc=0.0f;
    for(i=0;i<MAXVOICE;i++) if(g_voice[i].active) acc+=voice_sample(&g_voice[i]);
    acc+=bgm_sample(); /* BGM 레이어 (§16.4) */
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
#define MAXENE 256  /* 풀 상한(.bss, 파일크기 무관) — OVERCLOCK 256 호드 (docs/07 §14.6). DESCENT는 웨이브 캡상 실사용 ≤수십 */
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
#define ST_CODEX 5
#define ST_ENDING 6
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
#define M_CRYO 26
#define M_EXECUTE 27
#define M_DASHEDGE 28
#define MODN 29
#define MOD_COMMON 18
static int g_mod[MODN];
static int g_upgSel[3], g_upgRare, g_shotCount, g_leechKills, g_shieldUp, g_siphonK;
static float g_regenT, g_shieldT, g_thornT, g_slowT, g_slowAcc, g_frenzy;
static const char* g_modName[MODN]={"POWER","RAPID","AGILITY","MULTISHOT","PIERCE","HOMING","LEECH","EXPLOSIVE","RICOCHET","COOLANT","THORNS","CRITICAL","SWIFT SHOT","MAGNET","VENGEANCE","KINETIC","FRENZY","SIPHON","SPLITSHOT","TIMEWARP","OVERLOAD","REGEN","AEGIS","EXTRA CORE","GLASS CANNON","ARC CHAIN","CRYO ROUNDS","EXECUTE","PHASE EDGE"};
static const char* g_modDesc[MODN]={"DMG +25","FIRE RATE +20","MOVE SPD +12","SHOT +1","PIERCE +1","SEEK TARGETS","KILLS HEAL HP","HITS EXPLODE","WALL BOUNCE +2","DASH CD -25","CONTACT DMG","CRIT +15","BULLET SPD +30","PICKUP RANGE UP","LOW HP = DMG","SPD = DMG","HOLD RAMPS ROF","KILLS DROP BITS","HITS SPLIT 2","HURT SLOWMO","10TH SHOT X4","HP OVER TIME","BLOCK SHIELD","MAX HP +2 HEAL","+60% DMG RISK","HIT ARCS DMG","HITS SLOW FOES","LOW HP = LETHAL","DASH DEALS DMG"};
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
    int w[MOD_COMMON], total=0, n=rare?(MODN-MOD_COMMON):MOD_COMMON;
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
      } else { w[i]=10; /* 레어 균등, 단 과중첩 무의미 추첨 방지 캡 */
        if(mi==M_GLASSCANNON&&g_mod[M_GLASSCANNON]>=2) w[i]=0;      /* GC 캡2 */
        if(mi==M_EXECUTE&&g_mod[M_EXECUTE]>=3) w[i]=0;             /* 처형 임계 22% 상한(리뷰 LOW#7) */
        if(mi==M_CRYO&&g_mod[M_CRYO]>=4) w[i]=0;                   /* 둔화 지속 1.7s 상한 */
      }
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
/* 상점 (§13 스트레치): 비보스 레이어 35% 출현, 제단 3종(모듈/수리/리롤), E 구매 */
static int g_shopGX=-1, g_shopGY=-1, g_shopPrice[3]; static unsigned char g_shopBought[3];
static int pal_tier(void){ return (g_depth-1)%5; }
/* ---- 절차 BGM (§16.4): 120BPM, 팰릿 티어 루트, 아르페지오(sine)+베이스(saw)+킥. 레이어 시드 변주 ---- */
static int g_bgm=1, g_bgmIntense; static unsigned int g_bgmT; static float g_bgmArp, g_bgmBass, g_bgmKick;
static const float g_semi[13]={1.0f,1.05946f,1.12246f,1.18921f,1.25992f,1.33484f,1.41421f,1.49831f,1.58740f,1.68179f,1.78180f,1.88775f,2.0f};
static const int g_arpScale[6]={0,3,5,7,10,12}; /* 마이너 펜타토닉 + 옥타브 */
static const float g_bgmRoot[5]={55.0f,49.0f,61.7f,43.7f,65.4f}; /* 티어별 루트(A1/G1/B1/F1/C2) */
static float bgm_sample(void){
  if(!g_bgm) return 0.0f;
  if(!(g_state==ST_PLAY||g_state==ST_UPG||g_state==ST_PAUSE)) return 0.0f; /* 게임플레이 중만 */
  unsigned int spb=g_bgmIntense?20045u:22050u, step8=spb/2u; /* 120BPM, 보스 P3 시 +10% 템포 (§16.4) */
  unsigned int t=g_bgmT++;
  float root=g_bgmRoot[pal_tier()]*(g_bgmIntense?1.03f:1.0f); /* P3 디튠 */
  unsigned int eighth=t/step8, bar=eighth>>3, seed=g_master^(bar*2654435761u);
  int idx=(int)((eighth+(seed>>5))%6u);
  float af=root*2.0f*g_semi[g_arpScale[idx]];
  g_bgmArp+=af/(float)SND_RATE; if(g_bgmArp>=1.0f)g_bgmArp-=1.0f;
  float et=(float)(t%step8)/(float)SND_RATE, ae=et<0.005f?et/0.005f:(1.0f-(et-0.005f)/0.245f); if(ae<0.0f)ae=0.0f;
  float arp=osc_sin(g_bgmArp)*0.10f*ae;
  g_bgmBass+=root/(float)SND_RATE; if(g_bgmBass>=1.0f)g_bgmBass-=1.0f;
  float bass=osc_saw(g_bgmBass)*0.06f;
  unsigned int beat=t/spb, inbeat=t%spb; float kick=0.0f;
  if((beat&1u)==0u){ float kt=(float)inbeat/(float)SND_RATE; /* 비트 1·3 킥 */
    if(kt<0.1f){ float kf=120.0f-80.0f*(kt/0.1f); g_bgmKick+=kf/(float)SND_RATE; if(g_bgmKick>=1.0f)g_bgmKick-=1.0f; kick=osc_sin(g_bgmKick)*0.45f*(1.0f-kt/0.1f); } }
  return (arp+bass+kick)*0.5f;
}

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
static int g_wantDash, g_wantEmp, g_wantBuy; /* 입력 래치: 히트스톱/슬로모/고주사율 프레임에서 유실 방지 */
static int g_mapZoom; /* 미니맵 Tab 확대 (§18 스트레치) */

/* 점수/BITS (§13) */
static int g_kills, g_bits, g_bossKills, g_bestScore, g_bestLayer;
static int g_corruption, g_bestTime; static unsigned int g_codex; /* 메타: 누적 부패도·코덱스 해금 비트(§08 N2) */
static int score_now(void){ return g_depth*1000 + g_kills*25 + g_bits*5 + g_bossKills*500; }

/* juice (§14) */
static float g_trauma, g_hitstop, g_hurtFx, g_clearFx, g_fringeFx; /* g_fringeFx: 색수차 트리거(시각 전용) */
static vec2 g_kick;
static void add_trauma(float t){ g_trauma+=t; if(g_trauma>1.0f)g_trauma=1.0f; }

typedef struct { vec2 pos, vel; float life, dmg, r; unsigned char active, pierce, bounce, mini, lastHit; } Bullet;
/* 적 타입: 0헌터 1터릿 2리코셰 3포크 4샤드. affix 비트: 1보호막 2신속 4폭발사망 8자가분열 16자성 (§8.5) */
typedef struct { vec2 pos, vel, kvel; float hp, maxhp, t, t2, flash, spawn, at, at2, slowT; unsigned char active, type, affix, eshield; } Enemy;
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
/* 보스 활동 경계+중심 (docs/07 §14.3 — 양 모드 공유, 셀 결합 제거) */
static float g_bossMnx, g_bossMxx, g_bossMny, g_bossMxy, g_bossCx, g_bossCy;
/* ===== OVERCLOCK MODE 전역 (docs/07 SDD v0.4 — O1 MVP) ===== */
#define MODE_DESCENT 0
#define MODE_OVERCLOCK 1
#define W_PULSE 0
#define W_ORBIT 1
#define W_NOVA 2
#define W_AURA 3
#define W_SWARM 4
#define W_BEAM 5
#define W_ARC 6
#define W_DRONE 7
#define WEAPN 8
#define WEAP_CAP 8
static int g_mode;                 /* 0=DESCENT 1=OVERCLOCK */
static int g_ocLevel, g_ocXP, g_ocXPneed, g_ocBossCount, g_upgCtx; /* g_upgCtx: 0=모듈 draw3 / 1=OC 드래프트 */
static float g_ocT, g_ocSpawnT, g_ocBossT, g_orbAng, g_beamAng;
static unsigned char g_weapLvl[WEAPN], g_weapEvo[WEAPN]; static float g_weapTimer[WEAPN];
/* 무기 메타 단일 테이블 (확장: 무기 추가 = 한 줄). name/desc + evoReq(D-EVOLVE §4.4 짝 패시브). 인덱스=W_* */
typedef struct { const char *name, *desc; int evoReq; } WeapMeta;
static const WeapMeta g_weapMeta[WEAPN]={
  {"PULSE",    "AUTO BOLTS +1", M_PIERCE}, /* W_PULSE */
  {"ORBITERS", "ORBIT BLADES",  M_COOL},   /* W_ORBIT */
  {"NOVA",     "SEEKING BLAST", M_POWER},  /* W_NOVA  */
  {"AURA",     "DAMAGE FIELD",  M_THORNS}, /* W_AURA  */
  {"SWARM",    "KNIFE VOLLEY",  M_MULTI},  /* W_SWARM */
  {"BEAM",     "SWEEP LASER",   M_RAPID},  /* W_BEAM  */
  {"ARC",      "CHAIN BOLT",    M_CRIT},   /* W_ARC   */
  {"DRONES",   "AUTO DRONES",   M_HOMING}, /* W_DRONE */
};
static float g_arenaX0, g_arenaY0, g_arenaX1, g_arenaY1;
static int oc_score(void){ return (int)g_ocT*10 + g_kills*5 + g_ocLevel*200 + g_bossKills*1000; }
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
/* 적 타입별 스탯 단일 테이블 (확장: 적 추가 = 한 줄). r=반경 spd=기본속도 hp=기본HP, cr/cg/cb=색
 * 인덱스 = 적 타입(0헌터 1터릿 2리코셰 3포크 4샤드 5랜서 6위버 7팬텀). ene_radius/speed/color·spawn_enemy가 공유 */
typedef struct { float r, spd, hp, cr, cg, cb; } EneStat;
static const EneStat g_eneStat[8]={
  {13.0f,130.0f,20.0f, 1.00f,0.30f,0.45f}, /* 0 HUNTER   핑크 */
  {14.0f, 70.0f,16.0f, 0.30f,0.75f,1.00f}, /* 1 TURRET   시안 */
  {11.0f,230.0f,12.0f, 1.00f,0.80f,0.20f}, /* 2 RICOCHET 앰버 */
  {18.0f, 60.0f,30.0f, 0.45f,1.00f,0.40f}, /* 3 FORK     그린 */
  { 8.0f,160.0f, 8.0f, 0.45f,1.00f,0.40f}, /* 4 SHARD    그린 */
  {12.0f,110.0f,18.0f, 1.00f,0.45f,0.55f}, /* 5 LANCER   핫핑크 */
  {13.0f, 95.0f,22.0f, 0.65f,0.45f,1.00f}, /* 6 WEAVER   바이올렛 */
  {12.0f, 95.0f,18.0f, 0.82f,0.78f,1.00f}, /* 7 PHANTOM  스펙트럴(v1.5) */
};
static float ene_radius(int t){ return g_eneStat[t].r; }
static float ene_speed(int t){
  float sc=1.0f+0.04f*(float)(g_depth-1); if(sc>1.6f)sc=1.6f; return g_eneStat[t].spd*sc;
}
static void ene_color(int t,float*r,float*g,float*b){ *r=g_eneStat[t].cr; *g=g_eneStat[t].cg; *b=g_eneStat[t].cb; }

