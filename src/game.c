/* NEON DESCENT — M1+M2+M3+M4 (no-CRT Win32 + OpenGL 즉시모드)
 * rules/10·20·30. DESIGN §5·§6·§8·§10·§11·§13·§14·§17·§18·§21.
 * M4: 상태머신(TITLE/PLAYING/PAUSE/GAMEOVER), 3x5 폰트, HUD, 점수/BITS,
 *     대시(이동기)+잔상, juice(셰이크·히트스톱·플래시·넉백·텔레그래프).
 */
#include <windows.h>
#include <gl/gl.h>
#include <xmmintrin.h>

/* 영상 녹화: cl /D ND_REC 빌드 시 glReadPixels 프레임 덤프(rec_frames.bin) 활성. 릴리스 빌드엔 미포함. */

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


/* ==== 도메인 분할: 단일 TU 유지(#include), static 그대로. 빌드는 game.c만 컴파일 ==== */
#include "nd_data.inc"
#include "nd_meta.inc"
#include "nd_world.inc"
#include "nd_entity.inc"
#include "nd_combat.inc"
#include "nd_render.inc"
#ifdef ND_REC
/* 디버그 영상 덤프 (glReadPixels — 컴파일 게이트, 릴리스 빌드 무영향). cl /D ND_REC */
#define REC_MAX 96
static HANDLE g_recFile; static int g_recN; static unsigned char* g_recBuf; static unsigned char g_recLine[1920*3];
static int g_recSkip;
static void rec_frame(int w,int h){
  if((g_recSkip++ %3)!=0) return; /* 3프레임당 1캡처 → 더 긴 플레이 구간 */
  if(g_recN>=REC_MAX) return;
  if(!g_recFile){
    g_recFile=CreateFileA("rec_frames.bin",GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(g_recFile==INVALID_HANDLE_VALUE){ g_recFile=0; return; }
    g_recBuf=(unsigned char*)VirtualAlloc(0,(SIZE_T)w*h*3,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    DWORD wr; unsigned int hdr[3]; hdr[0]=(unsigned)(w/2); hdr[1]=(unsigned)(h/2); hdr[2]=REC_MAX;
    WriteFile(g_recFile,hdr,sizeof(hdr),&wr,0);
  }
  if(!g_recBuf) return;
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,g_recBuf);
  int dw=w/2, dh=h/2, x,y; DWORD wr;
  for(y=0;y<dh;y++){ int sy=(dh-1-y)*2; /* glReadPixels는 bottom-up → 수직 반전 */
    for(x=0;x<dw;x++){ unsigned char*p=&g_recBuf[((sy*w)+x*2)*3]; g_recLine[x*3]=p[0]; g_recLine[x*3+1]=p[1]; g_recLine[x*3+2]=p[2]; }
    WriteFile(g_recFile,g_recLine,(DWORD)(dw*3),&wr,0);
  }
  g_recN++;
  if(g_recN>=REC_MAX){ CloseHandle(g_recFile); g_recFile=0; }
}
#endif

/* F11 전체화면 토글 (§18·§19) — 보더리스, GL 컨텍스트 재생성 없음(매 프레임 glViewport가 적응) */
static void toggle_fullscreen(HWND hwnd){
  static RECT saved; static int fs=0;
  if(!fs){ GetWindowRect(hwnd,&saved);
    SetWindowLongPtrA(hwnd,GWL_STYLE,(LONG_PTR)(WS_POPUP|WS_VISIBLE));
    int sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd,HWND_TOP,0,0,sw,sh,SWP_FRAMECHANGED|SWP_SHOWWINDOW); fs=1;
  } else {
    SetWindowLongPtrA(hwnd,GWL_STYLE,(LONG_PTR)(WS_OVERLAPPEDWINDOW|WS_VISIBLE));
    SetWindowPos(hwnd,HWND_TOP,saved.left,saved.top,saved.right-saved.left,saved.bottom-saved.top,SWP_FRAMECHANGED|SWP_SHOWWINDOW); fs=0;
  }
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
#ifdef ND_REC
  /* 녹화: 창을 (0,0)로 이동 + 최상위 → GL 픽셀 오너십 보장(occlusion 시 glReadPixels undefined) */
  SetWindowPos(hwnd,(HWND)-1,0,0,1280,720,0); SetForegroundWindow(hwnd);
#endif

  LARGE_INTEGER li; QueryPerformanceCounter(&li); g_master=(unsigned int)li.QuadPart | 1u; g_rng=g_master; (void)xrnd(); g_rngFx=g_master^0x9E3779B9u; if(!g_rngFx)g_rngFx=0x12345678u;
  g_player.radius=14.0f; g_depth=1;
  save_load(); /* 메타 세이브 로드 (§08 N2) */
  snd_init();

  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);

  LARGE_INTEGER freq, prev, now; QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&prev);
  const float STEP=1.0f/120.0f; float acc=0.0f; MSG msg;
  while(!g_quit){
    while(PeekMessageA(&msg,0,0,0,PM_REMOVE)){ if(msg.message==WM_QUIT) g_quit=1; TranslateMessage(&msg); DispatchMessageA(&msg); }
    QueryPerformanceCounter(&now);
    float dt=(float)((double)(now.QuadPart-prev.QuadPart)/(double)freq.QuadPart); prev=now; if(dt>0.25f) dt=0.25f;

    if(g_kpress[VK_F11]){ g_kpress[VK_F11]=0; toggle_fullscreen(hwnd); } /* 전체화면 토글 (§18·§19) */

    /* 상태 전이 (§17) */
    if(g_state==ST_TITLE){
      if(g_kpress['A']||g_kpress[VK_LEFT]){ g_kpress['A']=0; g_kpress[VK_LEFT]=0; g_persona=(g_persona+3)%4; } /* 페르소나 순환 */
      if(g_kpress['D']||g_kpress[VK_RIGHT]){ g_kpress['D']=0; g_kpress[VK_RIGHT]=0; g_persona=(g_persona+1)%4; }
      if(g_kpress[VK_TAB]){ g_kpress[VK_TAB]=0; g_mode^=1; } /* 모드 전환 (DESCENT/OVERCLOCK) */
      if((g_mousePressed||g_kpress[VK_SPACE]||g_kpress[VK_RETURN])&&perso_unlocked(g_persona)){ g_kpress[VK_SPACE]=0; g_kpress[VK_RETURN]=0; new_run(); }
      if(g_kpress['C']){ g_kpress['C']=0; g_state=ST_CODEX; } /* 코덱스 뷰어 진입 */
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
      if(g_kpress['E']){ g_kpress['E']=0; g_wantBuy=1; } /* 상점 구매 (§13) */
      if(g_kpress[VK_TAB]){ g_kpress[VK_TAB]=0; g_mapZoom=!g_mapZoom; } /* 미니맵 확대 (§18) */
      if(g_xmitMsg) g_xmitT+=dt; /* 서사 교신 타이머 (표시 전용) */
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
      if(pick>=0&&g_upgSel[pick]>=0){ if(g_upgCtx==1) oc_apply(g_upgSel[pick]); else apply_mod(g_upgSel[pick]); }
      g_time+=dt; acc=0.0f;
    } else if(g_state==ST_PAUSE){
      if(g_kpress[VK_ESCAPE]){ g_kpress[VK_ESCAPE]=0; g_state=ST_PLAY; }
      if(g_kpress['Q']){ g_kpress['Q']=0; g_state=ST_TITLE; memset(g_part,0,sizeof(g_part)); g_time=0; }
      if(g_kpress['S']){ g_kpress['S']=0; g_optShake-=0.5f; if(g_optShake<0.0f)g_optShake=1.0f; }
      if(g_kpress['F']){ g_kpress['F']=0; g_optFlash-=0.5f; if(g_optFlash<0.0f)g_optFlash=1.0f; }
      if(g_kpress['C']){ g_kpress['C']=0; g_optCRT-=0.5f; if(g_optCRT<0.0f)g_optCRT=1.0f; }
      if(g_kpress['M']){ g_kpress['M']=0; g_mute=!g_mute; }
      if(g_kpress['B']){ g_kpress['B']=0; g_bgm=!g_bgm; } /* BGM 토글 (§16.4) */
      acc=0.0f;
    } else if(g_state==ST_CODEX){ /* 코덱스 뷰어 */
      if(g_kpress[VK_ESCAPE]||g_kpress['C']){ g_kpress[VK_ESCAPE]=0; g_kpress['C']=0; g_state=ST_TITLE; }
      g_time+=dt; acc=0.0f;
    } else if(g_state==ST_ENDING){ /* 엔딩 (§08 N3b) */
      if(g_kpress['R']){ g_kpress['R']=0; new_run(); }
      if(g_kpress[VK_ESCAPE]){ g_kpress[VK_ESCAPE]=0; g_state=ST_TITLE; memset(g_part,0,sizeof(g_part)); g_time=0; }
      g_time+=dt; acc=0.0f;
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
    else if(g_state==ST_PLAY){ camera_update(dt); render_world(w,h); if(g_mode==MODE_OVERCLOCK) oc_render_hud(w,h); else render_hud(w,h); render_post(w,h); }
    else if(g_state==ST_UPG){ render_upgrade(w,h); render_post(w,h); }
    else if(g_state==ST_PAUSE){ render_pause(w,h); render_post(w,h); }
    else if(g_state==ST_CODEX){ render_codex(w,h); }
    else if(g_state==ST_ENDING){ render_ending(w,h); }
    else { render_gameover(w,h); render_post(w,h); }
#ifdef ND_REC
    if(g_state==ST_PLAY) rec_frame(w,h);   /* SwapBuffers 전: 백버퍼에 렌더 결과 보존 (GL_BACK 읽기) */
#endif
    SwapBuffers(dc);
    snd_update();
  }
  if(g_sndOn){ int si; waveOutReset(g_wo);
    for(si=0;si<SND_BUFS;si++) waveOutUnprepareHeader(g_wo,&g_whdr[si],sizeof(WAVEHDR));
    waveOutClose(g_wo); }
  ExitProcess(0);
}
