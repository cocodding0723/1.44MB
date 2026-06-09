/* NEON DESCENT — M1+M2 (no-CRT Win32 + OpenGL 즉시모드)
 * rules/10(용량)·20(no-CRT)·30(품질). DESIGN §5·§10·§11·§14·§20·§21.
 * M1: 창/GL/고정 타임스텝/입력/PRNG/sqrt, 플레이어 이동+벽충돌.
 * M2: 절차 던전(random-walk)+타일 충돌+카메라 추적+레이어 하강(다운링크).
 */
#include <windows.h>
#include <gl/gl.h>
#include <xmmintrin.h>

int _fltused = 0;

/* no-CRT memset (배열 클리어용; #pragma function 로 재귀 인입 방지) */
#pragma function(memset)
void *memset(void *d, int v, size_t n){ unsigned char *p=(unsigned char*)d; while(n--) *p++=(unsigned char)v; return d; }

/* ---- 수학 ---- */
static float f_sqrt(float x){ return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x))); }
typedef struct { float x, y; } vec2;

/* ---- RNG: xorshift32 ---- */
static unsigned int g_rng = 2463534242u;
static unsigned int g_master = 1u;
static unsigned int xrnd(void){ unsigned int x=g_rng; x^=x<<13; x^=x>>17; x^=x<<5; g_rng=x; return x; }

/* ---- 입력/창 ---- */
static unsigned char g_keys[256];
static int g_mouseX, g_mouseY, g_winW=1280, g_winH=720, g_quit;

/* ---- 상수 (DESIGN §21) ---- */
#define MAXSPEED 220.0f
#define ACCEL    2000.0f
#define FRICTION 2400.0f
#define GW 9
#define GH 9
#define CELLT 22
#define MAPW (GW*CELLT)
#define MAPH (GH*CELLT)
#define TILEF 48.0f
#define SGX 4
#define SGY 4

/* ---- 던전 ---- */
static unsigned char g_tiles[MAPH][MAPW];          /* 0=벽 1=바닥, .bss */
typedef struct { unsigned char placed, type; int rx, ry, rw, rh, dist; } Cell; /* type 0시작 1전투 2모듈 3다운링크 */
static Cell g_grid[GH][GW];
static unsigned char g_conn[GH][GW];               /* bit1=up 2=right 4=down 8=left */
static int g_downGX=SGX, g_downGY=SGY, g_depth=1;

/* ---- 플레이어/카메라 ---- */
typedef struct { vec2 pos, vel; float radius; } Player;
static Player g_player;
static vec2 g_cam;

static const PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1,
  PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,32,
  0,0,0,0,0,0,0,0,0,0,0,0,0,24,8,0,PFD_MAIN_PLANE,0,0,0,0 };

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
  memset(g_tiles,0,sizeof(g_tiles));
  memset(g_grid,0,sizeof(g_grid));
  memset(g_conn,0,sizeof(g_conn));

  int N=4+g_depth; if(N>12)N=12;

  /* random-walk 방 배치 (연결성 보장: 새 방은 항상 기존 방에 연결) */
  int px[81], py[81], np=1, count=1, guard=0;
  g_grid[SGY][SGX].placed=1; px[0]=SGX; py[0]=SGY;
  while(count<N && guard<200000){
    guard++;
    int i=(int)(xrnd()%(unsigned)np);
    int cgx=px[i], cgy=py[i], d=(int)(xrnd()&3), nx=cgx, ny=cgy;
    if(d==0)ny--; else if(d==1)nx++; else if(d==2)ny++; else nx--;
    if(nx<0||ny<0||nx>=GW||ny>=GH) continue;
    if(!g_grid[ny][nx].placed){
      g_grid[ny][nx].placed=1; px[np]=nx; py[np]=ny; np++; count++;
      set_conn(cgx,cgy,nx,ny);
    } else if((xrnd()%100)<15){
      set_conn(cgx,cgy,nx,ny);
    }
  }

  /* 방 사각형 + 바닥 카브 */
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++){
    if(!g_grid[gy][gx].placed) continue;
    int rw=11+(int)(xrnd()%7), rh=9+(int)(xrnd()%5);
    int rx=gx*CELLT+(CELLT-rw)/2, ry=gy*CELLT+(CELLT-rh)/2;
    g_grid[gy][gx].rx=rx; g_grid[gy][gx].ry=ry; g_grid[gy][gx].rw=rw; g_grid[gy][gx].rh=rh; g_grid[gy][gx].type=1;
    for(y=ry;y<ry+rh;y++) for(x=rx;x<rx+rw;x++) put_floor(x,y);
  }

  /* 복도(중심-중심 L, 오른쪽/아래 연결만 처리해 중복 방지) */
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++){
    if(!g_grid[gy][gx].placed) continue;
    int acx=g_grid[gy][gx].rx+g_grid[gy][gx].rw/2, acy=g_grid[gy][gx].ry+g_grid[gy][gx].rh/2;
    if((g_conn[gy][gx]&2)&&gx+1<GW&&g_grid[gy][gx+1].placed)
      carve_corr(acx,acy,g_grid[gy][gx+1].rx+g_grid[gy][gx+1].rw/2,g_grid[gy][gx+1].ry+g_grid[gy][gx+1].rh/2);
    if((g_conn[gy][gx]&4)&&gy+1<GH&&g_grid[gy+1][gx].placed)
      carve_corr(acx,acy,g_grid[gy+1][gx].rx+g_grid[gy+1][gx].rw/2,g_grid[gy+1][gx].ry+g_grid[gy+1][gx].rh/2);
  }

  /* BFS 거리 */
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) g_grid[gy][gx].dist=-1;
  int qx[81], qy[81], qh=0, qt=0;
  g_grid[SGY][SGX].dist=0; qx[qt]=SGX; qy[qt]=SGY; qt++;
  while(qh<qt){
    int cgx=qx[qh], cgy=qy[qh]; qh++;
    unsigned char c=g_conn[cgy][cgx];
    int nb[4][2]={{cgx,cgy-1},{cgx+1,cgy},{cgx,cgy+1},{cgx-1,cgy}}, bit[4]={1,2,4,8};
    for(k=0;k<4;k++){
      if(!(c&bit[k])) continue;
      int nx=nb[k][0], ny=nb[k][1];
      if(nx<0||ny<0||nx>=GW||ny>=GH||!g_grid[ny][nx].placed||g_grid[ny][nx].dist>=0) continue;
      g_grid[ny][nx].dist=g_grid[cgy][cgx].dist+1; qx[qt]=nx; qy[qt]=ny; qt++;
    }
  }

  /* 다운링크=최장거리, 모듈=leaf */
  int bestd=-1, dgx=SGX, dgy=SGY;
  for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++)
    if(g_grid[gy][gx].placed&&g_grid[gy][gx].dist>bestd){ bestd=g_grid[gy][gx].dist; dgx=gx; dgy=gy; }
  g_grid[dgy][dgx].type=3; g_grid[SGY][SGX].type=0;
  for(gy=0;gy<GH;gy++){ for(gx=0;gx<GW;gx++){
    if(!g_grid[gy][gx].placed) continue;
    if((gx==SGX&&gy==SGY)||(gx==dgx&&gy==dgy)) continue;
    if(popcnt(g_conn[gy][gx])==1){ g_grid[gy][gx].type=2; gy=GH; break; }
  } }
  g_downGX=dgx; g_downGY=dgy;

  g_player.pos.x=(g_grid[SGY][SGX].rx+g_grid[SGY][SGX].rw*0.5f)*TILEF;
  g_player.pos.y=(g_grid[SGY][SGX].ry+g_grid[SGY][SGX].rh*0.5f)*TILEF;
  g_player.vel.x=0; g_player.vel.y=0; g_cam=g_player.pos;
}

static void descend(void){
  g_depth++;
  g_rng = g_master ^ ((unsigned)g_depth*2654435761u); if(!g_rng) g_rng=1;
  (void)xrnd(); (void)xrnd();
  generate();
}

static int is_wall_w(float wx, float wy){
  int tx=(int)(wx/TILEF), ty=(int)(wy/TILEF);
  if(tx<0||ty<0||tx>=MAPW||ty>=MAPH) return 1;
  return g_tiles[ty][tx]==0;
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l){
  switch(m){
    case WM_CLOSE: case WM_DESTROY: g_quit=1; PostQuitMessage(0); return 0;
    case WM_KEYDOWN: if(w<256) g_keys[w]=1; if(w==VK_ESCAPE){ g_quit=1; PostQuitMessage(0); } return 0;
    case WM_KEYUP:   if(w<256) g_keys[w]=0; return 0;
    case WM_MOUSEMOVE: g_mouseX=(short)LOWORD(l); g_mouseY=(short)HIWORD(l); return 0;
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

  /* 다운링크 도달 → 다음 레이어 */
  float dcx=(g_grid[g_downGY][g_downGX].rx+g_grid[g_downGY][g_downGX].rw*0.5f)*TILEF;
  float dcy=(g_grid[g_downGY][g_downGX].ry+g_grid[g_downGY][g_downGX].rh*0.5f)*TILEF;
  float ddx=g_player.pos.x-dcx, ddy=g_player.pos.y-dcy;
  if(ddx*ddx+ddy*ddy<2500.0f) descend();
}

static void camera_update(void){
  vec2 t=g_player.pos;
  float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
  float dx=mwx-g_player.pos.x, dy=mwy-g_player.pos.y, dl=f_sqrt(dx*dx+dy*dy);
  if(dl>1.0f){ float la=dl>60.0f?60.0f:dl; t.x+=dx/dl*la; t.y+=dy/dl*la; }
  g_cam.x+=(t.x-g_cam.x)*0.12f; g_cam.y+=(t.y-g_cam.y)*0.12f;
}

static void diamond(float cx,float cy,float r){
  glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0f,1.0f,1.0f); glVertex2f(cx,cy);
    glColor3f(0.0f,0.6f,0.8f);
    glVertex2f(cx,cy-r); glVertex2f(cx+r,cy); glVertex2f(cx,cy+r); glVertex2f(cx-r,cy); glVertex2f(cx,cy-r);
  glEnd();
}
static void diamond_line(float cx,float cy,float r){
  glBegin(GL_LINE_LOOP);
    glVertex2f(cx,cy-r); glVertex2f(cx+r,cy); glVertex2f(cx,cy+r); glVertex2f(cx-r,cy);
  glEnd();
}

static void render(int w,int h){
  int tx,ty;
  g_winW=w; g_winH=h; camera_update();
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  float L=g_cam.x-w*0.5f, R=g_cam.x+w*0.5f, T=g_cam.y-h*0.5f, B=g_cam.y+h*0.5f;
  glOrtho(L,R,B,T,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glClearColor(0.02f,0.03f,0.05f,1.0f); glClear(GL_COLOR_BUFFER_BIT);

  int tx0=(int)(L/TILEF)-1, tx1=(int)(R/TILEF)+1, ty0=(int)(T/TILEF)-1, ty1=(int)(B/TILEF)+1;
  if(tx0<0)tx0=0; if(ty0<0)ty0=0; if(tx1>MAPW-1)tx1=MAPW-1; if(ty1>MAPH-1)ty1=MAPH-1;

  glBegin(GL_QUADS); glColor3f(0.05f,0.08f,0.11f);
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){
    if(g_tiles[ty][tx]!=1) continue;
    float X=tx*TILEF, Y=ty*TILEF;
    glVertex2f(X,Y); glVertex2f(X+TILEF,Y); glVertex2f(X+TILEF,Y+TILEF); glVertex2f(X,Y+TILEF);
  }
  glEnd();

  glBegin(GL_LINES); glColor3f(0.0f,0.85f,1.0f);
  for(ty=ty0;ty<=ty1;ty++) for(tx=tx0;tx<=tx1;tx++){
    if(g_tiles[ty][tx]!=1) continue;
    float X=tx*TILEF, Y=ty*TILEF;
    if(ty==0||g_tiles[ty-1][tx]==0){ glVertex2f(X,Y); glVertex2f(X+TILEF,Y); }
    if(ty>=MAPH-1||g_tiles[ty+1][tx]==0){ glVertex2f(X,Y+TILEF); glVertex2f(X+TILEF,Y+TILEF); }
    if(tx==0||g_tiles[ty][tx-1]==0){ glVertex2f(X,Y); glVertex2f(X,Y+TILEF); }
    if(tx>=MAPW-1||g_tiles[ty][tx+1]==0){ glVertex2f(X+TILEF,Y); glVertex2f(X+TILEF,Y+TILEF); }
  }
  glEnd();

  /* 다운링크 마커 */
  float dcx=(g_grid[g_downGY][g_downGX].rx+g_grid[g_downGY][g_downGX].rw*0.5f)*TILEF;
  float dcy=(g_grid[g_downGY][g_downGX].ry+g_grid[g_downGY][g_downGX].rh*0.5f)*TILEF;
  glColor3f(0.2f,1.0f,0.85f); diamond_line(dcx,dcy,26.0f); diamond_line(dcx,dcy,16.0f);

  /* 모듈 마커(있으면) */
  { int gx,gy; for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) if(g_grid[gy][gx].placed&&g_grid[gy][gx].type==2){
      float mx=(g_grid[gy][gx].rx+g_grid[gy][gx].rw*0.5f)*TILEF, my=(g_grid[gy][gx].ry+g_grid[gy][gx].rh*0.5f)*TILEF;
      glColor3f(1.0f,0.3f,0.85f); diamond_line(mx,my,20.0f);
  } }

  diamond(g_player.pos.x,g_player.pos.y,g_player.radius);

  float mwx=L+(float)g_mouseX, mwy=T+(float)g_mouseY;
  glColor3f(1.0f,0.3f,0.6f);
  glBegin(GL_LINES);
    glVertex2f(mwx-9.0f,mwy); glVertex2f(mwx+9.0f,mwy); glVertex2f(mwx,mwy-9.0f); glVertex2f(mwx,mwy+9.0f);
  glEnd();
}

void WinMainCRTStartup(void){
  HINSTANCE inst=GetModuleHandleA(0);
  static WNDCLASSA wc;
  wc.style=CS_OWNDC; wc.lpfnWndProc=WndProc; wc.hInstance=inst;
  wc.hCursor=LoadCursorA(0,(LPCSTR)IDC_ARROW); wc.lpszClassName="ND";
  RegisterClassA(&wc);
  HWND hwnd=CreateWindowExA(0,"ND","NEON DESCENT",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
    CW_USEDEFAULT,CW_USEDEFAULT,1280,720,0,0,inst,0);
  HDC dc=GetDC(hwnd);
  SetPixelFormat(dc,ChoosePixelFormat(dc,&pfd),&pfd);
  wglMakeCurrent(dc,wglCreateContext(dc));
  ShowCursor(FALSE);

  LARGE_INTEGER li; QueryPerformanceCounter(&li);
  g_master=(unsigned int)li.QuadPart | 1u; g_rng=g_master; (void)xrnd();
  g_player.radius=14.0f; g_depth=1; generate();

  LARGE_INTEGER freq, prev, now; QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&prev);
  const float STEP=1.0f/120.0f; float acc=0.0f;
  MSG msg;
  while(!g_quit){
    while(PeekMessageA(&msg,0,0,0,PM_REMOVE)){ if(msg.message==WM_QUIT) g_quit=1; TranslateMessage(&msg); DispatchMessageA(&msg); }
    QueryPerformanceCounter(&now);
    float dt=(float)((double)(now.QuadPart-prev.QuadPart)/(double)freq.QuadPart); prev=now;
    if(dt>0.25f) dt=0.25f;
    acc+=dt;
    while(acc>=STEP){ player_update(STEP); acc-=STEP; }
    RECT rcl; GetClientRect(hwnd,&rcl);
    render(rcl.right,rcl.bottom);
    SwapBuffers(dc);
  }
  ExitProcess(0);
}
