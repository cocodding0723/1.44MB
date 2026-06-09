/* NEON DESCENT — M1 엔진 골격 (no-CRT Win32 + OpenGL 즉시모드)
 * rules/10(용량)·20(no-CRT)·30(품질). DESIGN §5(플레이어)·§20(기술)·§21(상수).
 * M1 범위: 창/GL/고정 타임스텝 루프/입력/PRNG/sqrt, 플레이어 이동+벽충돌.
 * (던전은 M2 — 여기선 방 경계 박스로 충돌 placeholder)
 */
#include <windows.h>
#include <gl/gl.h>
#include <xmmintrin.h>

int _fltused = 0;                          /* no-CRT: float 사용 표식 */

/* ---- 수학 (CRT 미사용, SSE 인트린식) ---- */
static float f_sqrt(float x){ return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x))); }
typedef struct { float x, y; } vec2;

/* ---- RNG: xorshift32 (rules/20) ---- */
static unsigned int g_rng = 2463534242u;
static unsigned int xrnd(void){ unsigned int x=g_rng; x^=x<<13; x^=x>>17; x^=x<<5; g_rng=x; return x; }

/* ---- 입력 상태 ---- */
static unsigned char g_keys[256];
static int g_mouseX, g_mouseY;
static int g_quit;

/* ---- 튜닝 상수 (DESIGN §21) ---- */
#define MAXSPEED  220.0f
#define ACCEL     2000.0f
#define FRICTION  2400.0f
#define ROOM_W    1200.0f                  /* M1 placeholder 방 크기 */
#define ROOM_H    680.0f

/* ---- 플레이어 ---- */
typedef struct { vec2 pos, vel; float radius; } Player;
static Player g_player;

static const PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1,
  PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER, PFD_TYPE_RGBA,32,
  0,0,0,0,0,0,0,0,0,0,0,0,0,24,8,0,PFD_MAIN_PLANE,0,0,0,0 };

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l){
  switch(m){
    case WM_CLOSE: case WM_DESTROY: g_quit=1; PostQuitMessage(0); return 0;
    case WM_KEYDOWN: if(w<256) g_keys[w]=1; if(w==VK_ESCAPE){ g_quit=1; PostQuitMessage(0); } return 0;
    case WM_KEYUP:   if(w<256) g_keys[w]=0; return 0;
    case WM_MOUSEMOVE: g_mouseX=(short)LOWORD(l); g_mouseY=(short)HIWORD(l); return 0;
  }
  return DefWindowProcA(h,m,w,l);
}

/* 고정 스텝 1/120 (DESIGN §20) */
static void player_update(float dt){
  vec2 in = {0.0f,0.0f};
  if(g_keys['W']||g_keys[VK_UP])    in.y -= 1.0f;
  if(g_keys['S']||g_keys[VK_DOWN])  in.y += 1.0f;
  if(g_keys['A']||g_keys[VK_LEFT])  in.x -= 1.0f;
  if(g_keys['D']||g_keys[VK_RIGHT]) in.x += 1.0f;

  float il = f_sqrt(in.x*in.x + in.y*in.y);
  if(il > 0.0001f){                          /* 입력: 정규화 후 가속 */
    in.x/=il; in.y/=il;
    g_player.vel.x += in.x*ACCEL*dt;
    g_player.vel.y += in.y*ACCEL*dt;
  } else {                                    /* 무입력: 마찰 감속 */
    float s = f_sqrt(g_player.vel.x*g_player.vel.x + g_player.vel.y*g_player.vel.y);
    if(s > 0.0001f){ float ns=s-FRICTION*dt; if(ns<0.0f)ns=0.0f;
      g_player.vel.x = g_player.vel.x/s*ns; g_player.vel.y = g_player.vel.y/s*ns; }
  }
  float sp = f_sqrt(g_player.vel.x*g_player.vel.x + g_player.vel.y*g_player.vel.y);
  if(sp > MAXSPEED){ g_player.vel.x=g_player.vel.x/sp*MAXSPEED; g_player.vel.y=g_player.vel.y/sp*MAXSPEED; }

  g_player.pos.x += g_player.vel.x*dt;
  g_player.pos.y += g_player.vel.y*dt;

  /* 벽 충돌 (방 경계, M2에서 던전 타일로 교체) */
  float r=g_player.radius;
  if(g_player.pos.x < r){ g_player.pos.x=r; g_player.vel.x=0.0f; }
  if(g_player.pos.y < r){ g_player.pos.y=r; g_player.vel.y=0.0f; }
  if(g_player.pos.x > ROOM_W-r){ g_player.pos.x=ROOM_W-r; g_player.vel.x=0.0f; }
  if(g_player.pos.y > ROOM_H-r){ g_player.pos.y=ROOM_H-r; g_player.vel.y=0.0f; }
}

static void draw_diamond(float cx, float cy, float r){
  glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0f,1.0f,1.0f); glVertex2f(cx,cy);          /* 흰 코어 */
    glColor3f(0.0f,0.6f,0.8f);                              /* 가장자리 글로우색 */
    glVertex2f(cx, cy-r); glVertex2f(cx+r, cy);
    glVertex2f(cx, cy+r); glVertex2f(cx-r, cy); glVertex2f(cx, cy-r);
  glEnd();
}

static void render(int w, int h){
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  glOrtho(0.0, ROOM_W, ROOM_H, 0.0, -1.0, 1.0);            /* 월드 좌표, y 아래 */
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glClearColor(0.03f,0.04f,0.06f,1.0f); glClear(GL_COLOR_BUFFER_BIT);

  glColor3f(0.0f,0.9f,1.0f);                                /* 방 경계(네온) */
  glBegin(GL_LINE_LOOP);
    glVertex2f(2.0f,2.0f); glVertex2f(ROOM_W-2.0f,2.0f);
    glVertex2f(ROOM_W-2.0f,ROOM_H-2.0f); glVertex2f(2.0f,ROOM_H-2.0f);
  glEnd();

  draw_diamond(g_player.pos.x, g_player.pos.y, g_player.radius);

  float mx=(float)g_mouseX*ROOM_W/(float)w;                 /* 커스텀 십자선(마우스) */
  float my=(float)g_mouseY*ROOM_H/(float)h;
  glColor3f(1.0f,0.3f,0.6f);
  glBegin(GL_LINES);
    glVertex2f(mx-9.0f,my); glVertex2f(mx+9.0f,my);
    glVertex2f(mx,my-9.0f); glVertex2f(mx,my+9.0f);
  glEnd();
}

void WinMainCRTStartup(void){                               /* no-CRT 엔트리 */
  HINSTANCE inst = GetModuleHandleA(0);
  static WNDCLASSA wc;                                      /* static = .bss 0초기화(memset 불필요) */
  wc.style=CS_OWNDC; wc.lpfnWndProc=WndProc; wc.hInstance=inst;
  wc.hCursor=LoadCursorA(0,(LPCSTR)IDC_ARROW); wc.lpszClassName="ND";
  RegisterClassA(&wc);

  HWND hwnd=CreateWindowExA(0,"ND","NEON DESCENT",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
    CW_USEDEFAULT,CW_USEDEFAULT,1280,720,0,0,inst,0);
  HDC dc=GetDC(hwnd);
  SetPixelFormat(dc, ChoosePixelFormat(dc,&pfd), &pfd);
  wglMakeCurrent(dc, wglCreateContext(dc));
  ShowCursor(FALSE);

  LARGE_INTEGER li; QueryPerformanceCounter(&li); g_rng=(unsigned int)li.QuadPart | 1u;
  (void)xrnd();

  g_player.pos.x=ROOM_W*0.5f; g_player.pos.y=ROOM_H*0.5f;
  g_player.vel.x=0.0f; g_player.vel.y=0.0f; g_player.radius=14.0f;

  LARGE_INTEGER freq, prev, now;
  QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&prev);
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
    render(rcl.right, rcl.bottom);
    SwapBuffers(dc);
  }
  ExitProcess(0);
}
