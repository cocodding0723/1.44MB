/* NEON DESCENT — M1+M2+M3 (no-CRT Win32 + OpenGL 즉시모드)
 * rules/10·20·30. DESIGN §5·§6·§8·§10·§11·§14·§20·§21.
 * M1 엔진골격 / M2 던전 / M3 전투(발사체·적3종·데미지/사망/파티클·전투방).
 */
#include <windows.h>
#include <gl/gl.h>
#include <xmmintrin.h>

int _fltused = 0;

#pragma function(memset)
void *memset(void *d, int v, size_t n){ unsigned char *p=(unsigned char*)d; while(n--) *p++=(unsigned char)v; return d; }

static float f_sqrt(float x){ return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x))); }
typedef struct { float x, y; } vec2;

static unsigned int g_rng=2463534242u, g_master=1u;
static unsigned int xrnd(void){ unsigned int x=g_rng; x^=x<<13; x^=x>>17; x^=x<<5; g_rng=x; return x; }
static float rnd01(void){ return (float)(xrnd()&0xFFFFFF)/(float)0xFFFFFF; }
static float rndsym(void){ return rnd01()*2.0f-1.0f; }

static unsigned char g_keys[256];
static int g_mouseX, g_mouseY, g_mouseDown, g_winW=1280, g_winH=720, g_quit;

/* 상수 (DESIGN §21) */
#define MAXSPEED 220.0f
#define ACCEL 2000.0f
#define FRICTION 2400.0f
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
#define MAXPART 512

static unsigned char g_tiles[MAPH][MAPW];
typedef struct { unsigned char placed, type; int rx, ry, rw, rh, dist; } Cell;
static Cell g_grid[GH][GW];
static unsigned char g_conn[GH][GW], g_cleared[GH][GW];
static int g_downGX=SGX, g_downGY=SGY, g_depth=1, g_locked, g_lockGX, g_lockGY;

typedef struct { vec2 pos, vel; float radius; } Player;
static Player g_player;
static vec2 g_cam;
static float g_pHP=6.0f, g_pMaxHP=6.0f, g_pIfr, g_fireCd;

typedef struct { vec2 pos, vel; float life; unsigned char active; } Bullet;
typedef struct { vec2 pos, vel; float hp, t, t2; unsigned char active, type; } Enemy; /* 0헌터 1터릿 2리코셰 */
typedef struct { vec2 pos, vel; float life, ilife, r, cr, cg, cb; unsigned char active; } Part;
static Bullet g_bul[MAXBUL], g_ebul[MAXEBUL];
static Enemy g_ene[MAXENE];
static Part g_part[MAXPART];

static const PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1,
  PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,32,
  0,0,0,0,0,0,0,0,0,0,0,0,0,24,8,0,PFD_MAIN_PLANE,0,0,0,0 };

static int is_wall_w(float wx, float wy){
  int tx=(int)(wx/TILEF), ty=(int)(wy/TILEF);
  if(tx<0||ty<0||tx>=MAPW||ty>=MAPH) return 1;
  return g_tiles[ty][tx]==0;
}
static float ene_radius(int t){ return t==0?13.0f:(t==1?14.0f:11.0f); }
static float ene_speed(int t){ float s=t==0?130.0f:(t==1?70.0f:230.0f), sc=1.0f+0.04f*(float)(g_depth-1); if(sc>1.6f)sc=1.6f; return s*sc; }

/* ---- 파티클 ---- */
static void spawn_part(float x,float y,float vx,float vy,float life,float r,float cr,float cg,float cb){
  int i; for(i=0;i<MAXPART;i++) if(!g_part[i].active){ Part*p=&g_part[i];
    p->active=1; p->pos.x=x; p->pos.y=y; p->vel.x=vx; p->vel.y=vy; p->life=life; p->ilife=life; p->r=r; p->cr=cr; p->cg=cg; p->cb=cb; return; }
}
static void burst(float x,float y,int n,float spd,float cr,float cg,float cb){
  int i; for(i=0;i<n;i++){ float dx=rndsym(), dy=rndsym(), l=f_sqrt(dx*dx+dy*dy); if(l<0.01f)l=1.0f;
    float s=spd*(0.4f+rnd01()*0.6f);
    spawn_part(x,y,dx/l*s,dy/l*s,0.25f+rnd01()*0.35f,2.0f+rnd01()*3.0f,cr,cg,cb); }
}

/* ---- 던전 ---- */
static int popcnt(unsigned char v){ int c=0; while(v){ c+=v&1; v>>=1; } return c; }
static void set_conn(int ax,int ay,int bx,int by){
  if(bx==ax&&by==ay-1){ g_conn[ay][ax]|=1; g_conn[by][bx]|=4; }
  else if(bx==ax+1&&by==ay){ g_conn[ay][ax]|=2; g_conn[by][bx]|=8; }
  else if(bx==ax&&by==ay+1){ g_conn[ay][ax]|=4; g_conn[by][bx]|=1; }
  else if(bx==ax-1&&by==ay){ g_conn[ay][ax]|=8; g_conn[by][bx]|=2; }
}
static void put_floor(int x,int y){ if(x>=0&&y>=0&&x<MAPW&&y<MAPH) g_tiles[y][x]=1; }
static void carve_corr(int ax,int ay,int bx,int by){
  int x,y,x0=ax<bx?ax:bx,x1=ax<bx?bx:ax,y0=ay<by?ay:by,y1=ay<by?by:ay;
  for(x=x0;x<=x1;x++){ put_floor(x,ay); put_floor(x,ay+1); }
  for(y=y0;y<=y1;y++){ put_floor(bx,y); put_floor(bx+1,y); }
}
static void generate(void){
  int gx,gy,x,y,k;
  memset(g_tiles,0,sizeof(g_tiles)); memset(g_grid,0,sizeof(g_grid)); memset(g_conn,0,sizeof(g_conn));
  memset(g_cleared,0,sizeof(g_cleared));
  memset(g_ene,0,sizeof(g_ene)); memset(g_bul,0,sizeof(g_bul)); memset(g_ebul,0,sizeof(g_ebul)); memset(g_part,0,sizeof(g_part));
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
  g_player.pos.x=(g_grid[SGY][SGX].rx+g_grid[SGY][SGX].rw*0.5f)*TILEF;
  g_player.pos.y=(g_grid[SGY][SGX].ry+g_grid[SGY][SGX].rh*0.5f)*TILEF;
  g_player.vel.x=0; g_player.vel.y=0; g_cam=g_player.pos;
}
static void descend(void){ g_depth++; g_rng=g_master^((unsigned)g_depth*2654435761u); if(!g_rng)g_rng=1; (void)xrnd(); (void)xrnd(); generate(); }
static void respawn(void){ /* M3: 사망 시 현재 레이어 재생성(게임오버 UI는 M4) */ g_pHP=g_pMaxHP; g_rng=g_master^((unsigned)g_depth*40503u); if(!g_rng)g_rng=1; generate(); }

/* ---- 적 스폰 ---- */
static void spawn_enemy(int type,float x,float y){
  int i; for(i=0;i<MAXENE;i++) if(!g_ene[i].active){ Enemy*e=&g_ene[i];
    e->active=1; e->type=(unsigned char)type; e->pos.x=x; e->pos.y=y; e->vel.x=0; e->vel.y=0; e->t=0; e->t2=0;
    float hpsc=1.0f+0.18f*(float)(g_depth-1);
    if(type==0) e->hp=20.0f*hpsc; else if(type==1) e->hp=16.0f*hpsc;
    else { e->hp=12.0f*hpsc; float dx=rndsym(), dy=rndsym(), l=f_sqrt(dx*dx+dy*dy); if(l<0.01f)l=1.0f; float sp=ene_speed(2); e->vel.x=dx/l*sp; e->vel.y=dy/l*sp; }
    return; }
}
static void spawn_wave(int gx,int gy){
  Cell*c=&g_grid[gy][gx];
  int cnt=4+g_depth; if(cnt>12)cnt=12;
  int placed=0, tries=0;
  while(placed<cnt && tries<500){ tries++;
    int tx=c->rx+1+(int)(xrnd()%(unsigned)(c->rw-2)), ty=c->ry+1+(int)(xrnd()%(unsigned)(c->rh-2));
    float wx=(tx+0.5f)*TILEF, wy=(ty+0.5f)*TILEF, dx=wx-g_player.pos.x, dy=wy-g_player.pos.y;
    if(dx*dx+dy*dy<150.0f*150.0f) continue;
    spawn_enemy((int)(xrnd()%3),wx,wy);  /* M3: 3종 혼합(깊이 게이팅 §12는 M5) */
    placed++;
  }
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l){
  switch(m){
    case WM_CLOSE: case WM_DESTROY: g_quit=1; PostQuitMessage(0); return 0;
    case WM_KEYDOWN: if(w<256) g_keys[w]=1; if(w==VK_ESCAPE){ g_quit=1; PostQuitMessage(0); } return 0;
    case WM_KEYUP: if(w<256) g_keys[w]=0; return 0;
    case WM_MOUSEMOVE: g_mouseX=(short)LOWORD(l); g_mouseY=(short)HIWORD(l); return 0;
    case WM_LBUTTONDOWN: g_mouseDown=1; return 0;
    case WM_LBUTTONUP: g_mouseDown=0; return 0;
  }
  return DefWindowProcA(h,m,w,l);
}

static void player_update(float dt){
  vec2 in={0.0f,0.0f};
  if(g_keys['W']||g_keys[VK_UP])    in.y-=1.0f;
  if(g_keys['S']||g_keys[VK_DOWN])  in.y+=1.0f;
  if(g_keys['A']||g_keys[VK_LEFT])  in.x-=1.0f;
  if(g_keys['D']||g_keys[VK_RIGHT]) in.x+=1.0f;
  float il=f_sqrt(in.x*in.x+in.y*in.y);
  if(il>0.0001f){ in.x/=il; in.y/=il; g_player.vel.x+=in.x*ACCEL*dt; g_player.vel.y+=in.y*ACCEL*dt; }
  else { float s=f_sqrt(g_player.vel.x*g_player.vel.x+g_player.vel.y*g_player.vel.y);
    if(s>0.0001f){ float ns=s-FRICTION*dt; if(ns<0.0f)ns=0.0f; g_player.vel.x=g_player.vel.x/s*ns; g_player.vel.y=g_player.vel.y/s*ns; } }
  float sp=f_sqrt(g_player.vel.x*g_player.vel.x+g_player.vel.y*g_player.vel.y);
  if(sp>MAXSPEED){ g_player.vel.x=g_player.vel.x/sp*MAXSPEED; g_player.vel.y=g_player.vel.y/sp*MAXSPEED; }
  float r=g_player.radius;
  float nxp=g_player.pos.x+g_player.vel.x*dt, ex=nxp+(g_player.vel.x>0.0f?r:-r);
  if(is_wall_w(ex,g_player.pos.y-r*0.6f)||is_wall_w(ex,g_player.pos.y+r*0.6f)) g_player.vel.x=0.0f; else g_player.pos.x=nxp;
  float nyp=g_player.pos.y+g_player.vel.y*dt, ey=nyp+(g_player.vel.y>0.0f?r:-r);
  if(is_wall_w(g_player.pos.x-r*0.6f,ey)||is_wall_w(g_player.pos.x+r*0.6f,ey)) g_player.vel.y=0.0f; else g_player.pos.y=nyp;
}

static void hurt_player(float dmg){ if(g_pIfr>0.0f) return; g_pHP-=dmg; g_pIfr=1.0f; if(g_pHP<=0.0f) respawn(); }

static void combat_update(float dt){
  int i,j;
  g_fireCd-=dt; if(g_pIfr>0.0f) g_pIfr-=dt;

  /* 발사 (LMB 홀드) */
  if(g_mouseDown && g_fireCd<=0.0f){
    g_fireCd=FIRE_INT;
    float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
    float dx=mwx-g_player.pos.x, dy=mwy-g_player.pos.y, l=f_sqrt(dx*dx+dy*dy); if(l<0.001f)l=1.0f;
    for(i=0;i<MAXBUL;i++) if(!g_bul[i].active){ g_bul[i].active=1; g_bul[i].pos=g_player.pos; g_bul[i].vel.x=dx/l*BUL_SPEED; g_bul[i].vel.y=dy/l*BUL_SPEED; g_bul[i].life=BUL_LIFE; break; }
  }

  /* 플레이어 발사체 */
  for(i=0;i<MAXBUL;i++){ if(!g_bul[i].active) continue; Bullet*b=&g_bul[i];
    b->pos.x+=b->vel.x*dt; b->pos.y+=b->vel.y*dt; b->life-=dt;
    if(b->life<=0.0f||is_wall_w(b->pos.x,b->pos.y)){ b->active=0; continue; }
    for(j=0;j<MAXENE;j++){ if(!g_ene[j].active) continue; Enemy*e=&g_ene[j];
      float rr=ene_radius(e->type)+BUL_R, ddx=b->pos.x-e->pos.x, ddy=b->pos.y-e->pos.y;
      if(ddx*ddx+ddy*ddy<rr*rr){
        e->hp-=BUL_DMG; b->active=0; burst(b->pos.x,b->pos.y,6,160.0f,1.0f,1.0f,1.0f);
        if(e->hp<=0.0f){ e->active=0; burst(e->pos.x,e->pos.y,16,220.0f,1.0f,0.5f,0.2f); }
        break;
      }
    }
  }

  /* 적 AI */
  for(i=0;i<MAXENE;i++){ if(!g_ene[i].active) continue; Enemy*e=&g_ene[i];
    float r=ene_radius(e->type), spd=ene_speed(e->type);
    float dx=g_player.pos.x-e->pos.x, dy=g_player.pos.y-e->pos.y, dist=f_sqrt(dx*dx+dy*dy); if(dist<0.01f)dist=0.01f;
    e->t+=dt;
    if(e->type==0){ /* 헌터: 추격 */
      float vx=dx/dist*spd, vy=dy/dist*spd;
      float nx=e->pos.x+vx*dt, ex=nx+(vx>0?r:-r);
      if(!(is_wall_w(ex,e->pos.y-r*0.6f)||is_wall_w(ex,e->pos.y+r*0.6f))) e->pos.x=nx;
      float ny=e->pos.y+vy*dt, ey=ny+(vy>0?r:-r);
      if(!(is_wall_w(e->pos.x-r*0.6f,ey)||is_wall_w(e->pos.x+r*0.6f,ey))) e->pos.y=ny;
    } else if(e->type==1){ /* 터릿: 거리유지 + 발사 */
      float dir=0.0f; if(dist<300.0f) dir=-1.0f; else if(dist>340.0f) dir=1.0f;
      float vx=dx/dist*spd*dir, vy=dy/dist*spd*dir;
      float nx=e->pos.x+vx*dt, ex=nx+(vx>0?r:-r);
      if(!(is_wall_w(ex,e->pos.y-r*0.6f)||is_wall_w(ex,e->pos.y+r*0.6f))) e->pos.x=nx;
      float ny=e->pos.y+vy*dt, ey=ny+(vy>0?r:-r);
      if(!(is_wall_w(e->pos.x-r*0.6f,ey)||is_wall_w(e->pos.x+r*0.6f,ey))) e->pos.y=ny;
      e->t2+=dt;
      if(e->t2>=1.6f){ e->t2=0.0f;
        for(j=0;j<MAXEBUL;j++) if(!g_ebul[j].active){ g_ebul[j].active=1; g_ebul[j].pos=e->pos; g_ebul[j].vel.x=dx/dist*EBUL_SPEED; g_ebul[j].vel.y=dy/dist*EBUL_SPEED; g_ebul[j].life=2.5f; break; }
      }
    } else { /* 리코셰: 직선+반사, 1.5s 방향 재설정 */
      if(e->t>=1.5f){ e->t=0.0f; float rx=rndsym(), ry=rndsym();
        if((xrnd()%100)<30){ rx=dx/dist; ry=dy/dist; }
        float l=f_sqrt(rx*rx+ry*ry); if(l<0.01f)l=1.0f; e->vel.x=rx/l*spd; e->vel.y=ry/l*spd; }
      float nx=e->pos.x+e->vel.x*dt, ex=nx+(e->vel.x>0?r:-r);
      if(is_wall_w(ex,e->pos.y)) e->vel.x=-e->vel.x; else e->pos.x=nx;
      float ny=e->pos.y+e->vel.y*dt, ey=ny+(e->vel.y>0?r:-r);
      if(is_wall_w(e->pos.x,ey)) e->vel.y=-e->vel.y; else e->pos.y=ny;
    }
    /* 접촉 데미지 */
    float cr=r+g_player.radius;
    if(dx*dx+dy*dy<cr*cr) hurt_player(1.0f);
  }

  /* 적 발사체 */
  for(i=0;i<MAXEBUL;i++){ if(!g_ebul[i].active) continue; Bullet*b=&g_ebul[i];
    b->pos.x+=b->vel.x*dt; b->pos.y+=b->vel.y*dt; b->life-=dt;
    if(b->life<=0.0f||is_wall_w(b->pos.x,b->pos.y)){ b->active=0; continue; }
    float ddx=b->pos.x-g_player.pos.x, ddy=b->pos.y-g_player.pos.y, rr=g_player.radius+6.0f;
    if(ddx*ddx+ddy*ddy<rr*rr){ b->active=0; hurt_player(1.0f); }
  }

  /* 파티클 */
  for(i=0;i<MAXPART;i++){ if(!g_part[i].active) continue; Part*p=&g_part[i];
    p->pos.x+=p->vel.x*dt; p->pos.y+=p->vel.y*dt; p->vel.x*=0.92f; p->vel.y*=0.92f; p->life-=dt; if(p->life<=0.0f) p->active=0; }

  /* 전투방 잠금/전멸/개방 */
  int ptx=(int)(g_player.pos.x/TILEF), pty=(int)(g_player.pos.y/TILEF), cgx=ptx/CELLT, cgy=pty/CELLT;
  if(!g_locked && cgx>=0&&cgy>=0&&cgx<GW&&cgy<GH && g_grid[cgy][cgx].placed){
    Cell*c=&g_grid[cgy][cgx];
    if(c->type==1 && !g_cleared[cgy][cgx] && ptx>=c->rx&&ptx<c->rx+c->rw&&pty>=c->ry&&pty<c->ry+c->rh){
      g_locked=1; g_lockGX=cgx; g_lockGY=cgy; spawn_wave(cgx,cgy);
    }
  }
  if(g_locked){ Cell*c=&g_grid[g_lockGY][g_lockGX];
    float mnx=(c->rx+1)*TILEF, mxx=(c->rx+c->rw-1)*TILEF, mny=(c->ry+1)*TILEF, mxy=(c->ry+c->rh-1)*TILEF;
    if(g_player.pos.x<mnx)g_player.pos.x=mnx; if(g_player.pos.x>mxx)g_player.pos.x=mxx;
    if(g_player.pos.y<mny)g_player.pos.y=mny; if(g_player.pos.y>mxy)g_player.pos.y=mxy;
    /* 적도 방 안에 confine (리코셰 탈출로 인한 미클리어 방지 — 리코셰는 경계 반사) */
    for(j=0;j<MAXENE;j++){ if(!g_ene[j].active) continue; float er=ene_radius(g_ene[j].type);
      float emnx=c->rx*TILEF+er, emxx=(c->rx+c->rw)*TILEF-er, emny=c->ry*TILEF+er, emxy=(c->ry+c->rh)*TILEF-er;
      if(g_ene[j].pos.x<emnx){ g_ene[j].pos.x=emnx; if(g_ene[j].type==2) g_ene[j].vel.x=-g_ene[j].vel.x; }
      else if(g_ene[j].pos.x>emxx){ g_ene[j].pos.x=emxx; if(g_ene[j].type==2) g_ene[j].vel.x=-g_ene[j].vel.x; }
      if(g_ene[j].pos.y<emny){ g_ene[j].pos.y=emny; if(g_ene[j].type==2) g_ene[j].vel.y=-g_ene[j].vel.y; }
      else if(g_ene[j].pos.y>emxy){ g_ene[j].pos.y=emxy; if(g_ene[j].type==2) g_ene[j].vel.y=-g_ene[j].vel.y; }
    }
    int alive=0; for(i=0;i<MAXENE;i++) if(g_ene[i].active) alive++;
    if(alive==0){ g_cleared[g_lockGY][g_lockGX]=1; g_locked=0; burst(g_player.pos.x,g_player.pos.y,12,120.0f,0.2f,1.0f,0.9f); }
  }

  /* 다운링크 하강 (잠금 아닐 때만) */
  if(!g_locked){
    float dcx=(g_grid[g_downGY][g_downGX].rx+g_grid[g_downGY][g_downGX].rw*0.5f)*TILEF;
    float dcy=(g_grid[g_downGY][g_downGX].ry+g_grid[g_downGY][g_downGX].rh*0.5f)*TILEF;
    float ddx=g_player.pos.x-dcx, ddy=g_player.pos.y-dcy;
    if(ddx*ddx+ddy*ddy<2500.0f) descend();
  }
}

static void camera_update(void){
  vec2 t=g_player.pos;
  float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
  float dx=mwx-g_player.pos.x, dy=mwy-g_player.pos.y, dl=f_sqrt(dx*dx+dy*dy);
  if(dl>1.0f){ float la=dl>60.0f?60.0f:dl; t.x+=dx/dl*la; t.y+=dy/dl*la; }
  g_cam.x+=(t.x-g_cam.x)*0.12f; g_cam.y+=(t.y-g_cam.y)*0.12f;
}

static void poly_fill(float cx,float cy,int n,float r,float cr,float cg,float cb){
  /* n=3 삼각, 4 다이아/사각 — 여기선 다이아/삼각 외형용 라인루프 채움 근사 */
}
static void diamond(float cx,float cy,float r){
  glBegin(GL_TRIANGLE_FAN); glColor3f(1.0f,1.0f,1.0f); glVertex2f(cx,cy);
    glColor3f(0.0f,0.6f,0.8f); glVertex2f(cx,cy-r); glVertex2f(cx+r,cy); glVertex2f(cx,cy+r); glVertex2f(cx-r,cy); glVertex2f(cx,cy-r);
  glEnd();
}
static void diamond_line(float cx,float cy,float r){
  glBegin(GL_LINE_LOOP); glVertex2f(cx,cy-r); glVertex2f(cx+r,cy); glVertex2f(cx,cy+r); glVertex2f(cx-r,cy); glEnd();
}
static void draw_enemy(Enemy*e){
  float r=ene_radius(e->type), x=e->pos.x, y=e->pos.y;
  if(e->type==0){ glColor3f(1.0f,0.30f,0.45f); glBegin(GL_TRIANGLES); glVertex2f(x,y-r); glVertex2f(x+r*0.9f,y+r*0.7f); glVertex2f(x-r*0.9f,y+r*0.7f); glEnd(); }
  else if(e->type==1){ glColor3f(0.30f,0.75f,1.0f); glBegin(GL_QUADS); glVertex2f(x-r,y-r); glVertex2f(x+r,y-r); glVertex2f(x+r,y+r); glVertex2f(x-r,y+r); glEnd(); }
  else { glColor3f(1.0f,0.80f,0.20f); glBegin(GL_QUADS); glVertex2f(x,y-r); glVertex2f(x+r,y); glVertex2f(x,y+r); glVertex2f(x-r,y); glEnd(); }
}

static void render(int w,int h){
  int tx,ty,i;
  g_winW=w; g_winH=h; camera_update();
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  float L=g_cam.x-w*0.5f, R=g_cam.x+w*0.5f, T=g_cam.y-h*0.5f, B=g_cam.y+h*0.5f;
  glOrtho(L,R,B,T,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  glClearColor(0.02f,0.03f,0.05f,1.0f); glClear(GL_COLOR_BUFFER_BIT);

  int tx0=(int)(L/TILEF)-1, tx1=(int)(R/TILEF)+1, ty0=(int)(T/TILEF)-1, ty1=(int)(B/TILEF)+1;
  if(tx0<0)tx0=0; if(ty0<0)ty0=0; if(tx1>MAPW-1)tx1=MAPW-1; if(ty1>MAPH-1)ty1=MAPH-1;

  glBegin(GL_QUADS); glColor3f(0.05f,0.08f,0.11f);
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){ if(g_tiles[ty][tx]!=1) continue;
    float X=tx*TILEF, Y=ty*TILEF; glVertex2f(X,Y); glVertex2f(X+TILEF,Y); glVertex2f(X+TILEF,Y+TILEF); glVertex2f(X,Y+TILEF); }
  glEnd();

  glBegin(GL_LINES); glColor3f(0.0f,0.85f,1.0f);
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){ if(g_tiles[ty][tx]!=1) continue; float X=tx*TILEF, Y=ty*TILEF;
    if(ty==0||g_tiles[ty-1][tx]==0){ glVertex2f(X,Y); glVertex2f(X+TILEF,Y); }
    if(ty>=MAPH-1||g_tiles[ty+1][tx]==0){ glVertex2f(X,Y+TILEF); glVertex2f(X+TILEF,Y+TILEF); }
    if(tx==0||g_tiles[ty][tx-1]==0){ glVertex2f(X,Y); glVertex2f(X,Y+TILEF); }
    if(tx>=MAPW-1||g_tiles[ty][tx+1]==0){ glVertex2f(X+TILEF,Y); glVertex2f(X+TILEF,Y+TILEF); } }
  glEnd();

  float dcx=(g_grid[g_downGY][g_downGX].rx+g_grid[g_downGY][g_downGX].rw*0.5f)*TILEF, dcy=(g_grid[g_downGY][g_downGX].ry+g_grid[g_downGY][g_downGX].rh*0.5f)*TILEF;
  glColor3f(0.2f,1.0f,0.85f); diamond_line(dcx,dcy,26.0f); diamond_line(dcx,dcy,16.0f);
  { int gx,gy; for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) if(g_grid[gy][gx].placed&&g_grid[gy][gx].type==2){
      float mx=(g_grid[gy][gx].rx+g_grid[gy][gx].rw*0.5f)*TILEF, my=(g_grid[gy][gx].ry+g_grid[gy][gx].rh*0.5f)*TILEF;
      glColor3f(1.0f,0.3f,0.85f); diamond_line(mx,my,20.0f); } }

  /* 적 발사체 */
  glColor3f(1.0f,0.5f,0.2f); glBegin(GL_QUADS);
  for(i=0;i<MAXEBUL;i++){ if(!g_ebul[i].active) continue; float x=g_ebul[i].pos.x,y=g_ebul[i].pos.y,s=6.0f;
    glVertex2f(x-s,y-s); glVertex2f(x+s,y-s); glVertex2f(x+s,y+s); glVertex2f(x-s,y+s); }
  glEnd();

  /* 적 */
  for(i=0;i<MAXENE;i++) if(g_ene[i].active) draw_enemy(&g_ene[i]);

  /* 플레이어 발사체 */
  glColor3f(0.6f,1.0f,1.0f); glBegin(GL_QUADS);
  for(i=0;i<MAXBUL;i++){ if(!g_bul[i].active) continue; float x=g_bul[i].pos.x,y=g_bul[i].pos.y,s=BUL_R;
    glVertex2f(x-s,y-s); glVertex2f(x+s,y-s); glVertex2f(x+s,y+s); glVertex2f(x-s,y+s); }
  glEnd();

  /* 파티클 (가산, 페이드) */
  glBegin(GL_QUADS);
  for(i=0;i<MAXPART;i++){ if(!g_part[i].active) continue; Part*p=&g_part[i]; float a=p->life/p->ilife; if(a<0)a=0;
    glColor4f(p->cr,p->cg,p->cb,a); float x=p->pos.x,y=p->pos.y,s=p->r;
    glVertex2f(x-s,y-s); glVertex2f(x+s,y-s); glVertex2f(x+s,y+s); glVertex2f(x-s,y+s); }
  glEnd();

  /* 플레이어 (무적 깜빡임) */
  if(!(g_pIfr>0.0f && (((int)(g_pIfr*20.0f))&1))) diamond(g_player.pos.x,g_player.pos.y,g_player.radius);

  /* 십자선 */
  float mwx=L+(float)g_mouseX, mwy=T+(float)g_mouseY;
  glColor3f(1.0f,0.3f,0.6f); glBegin(GL_LINES);
    glVertex2f(mwx-9.0f,mwy); glVertex2f(mwx+9.0f,mwy); glVertex2f(mwx,mwy-9.0f); glVertex2f(mwx,mwy+9.0f); glEnd();

  /* HUD: HP (스크린 좌표) */
  glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,w,h,0,-1,1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glBegin(GL_QUADS);
  for(i=0;i<(int)g_pMaxHP;i++){ float bx=16.0f+i*22.0f, by=16.0f;
    if((float)i<g_pHP) glColor3f(1.0f,0.25f,0.4f); else glColor3f(0.18f,0.10f,0.14f);
    glVertex2f(bx,by); glVertex2f(bx+18.0f,by); glVertex2f(bx+18.0f,by+18.0f); glVertex2f(bx,by+18.0f); }
  glEnd();
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

  LARGE_INTEGER li; QueryPerformanceCounter(&li); g_master=(unsigned int)li.QuadPart | 1u; g_rng=g_master; (void)xrnd();
  g_player.radius=14.0f; g_depth=1; g_pHP=g_pMaxHP; generate();

  LARGE_INTEGER freq, prev, now; QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&prev);
  const float STEP=1.0f/120.0f; float acc=0.0f; MSG msg;
  while(!g_quit){
    while(PeekMessageA(&msg,0,0,0,PM_REMOVE)){ if(msg.message==WM_QUIT) g_quit=1; TranslateMessage(&msg); DispatchMessageA(&msg); }
    QueryPerformanceCounter(&now);
    float dt=(float)((double)(now.QuadPart-prev.QuadPart)/(double)freq.QuadPart); prev=now; if(dt>0.25f) dt=0.25f;
    acc+=dt;
    while(acc>=STEP){ player_update(STEP); combat_update(STEP); acc-=STEP; }
    RECT rcl; GetClientRect(hwnd,&rcl); render(rcl.right,rcl.bottom); SwapBuffers(dc);
  }
  ExitProcess(0);
}
