/* ---- 3x5 비트맵 폰트 (§15 — 파일 없음, 코드 인코딩) ---- */
#define GF(a,b,c,d,e) (unsigned short)(((a)<<12)|((b)<<9)|((c)<<6)|((d)<<3)|(e))
static const unsigned short g_font[44]={
  GF(7,5,5,5,7),GF(2,6,2,2,7),GF(7,1,7,4,7),GF(7,1,7,1,7),GF(5,5,7,1,1), /* 0-4 */
  GF(7,4,7,1,7),GF(7,4,7,5,7),GF(7,1,1,2,2),GF(7,5,7,5,7),GF(7,5,7,1,7), /* 5-9 */
  GF(2,5,7,5,5),GF(6,5,6,5,6),GF(7,4,4,4,7),GF(6,5,5,5,6),GF(7,4,7,4,7), /* A-E */
  GF(7,4,7,4,4),GF(7,4,5,5,7),GF(5,5,7,5,5),GF(7,2,2,2,7),GF(1,1,1,5,7), /* F-J */
  GF(5,5,6,5,5),GF(4,4,4,4,7),GF(5,7,7,5,5),GF(6,5,5,5,5),GF(7,5,5,5,7), /* K-O */
  GF(7,5,7,4,4),GF(7,5,5,7,1),GF(7,5,6,5,5),GF(7,4,7,1,7),GF(7,2,2,2,2), /* P-T */
  GF(5,5,5,5,7),GF(5,5,5,5,2),GF(5,5,7,7,5),GF(5,5,2,5,5),GF(5,5,2,2,2), /* U-Y */
  GF(7,1,2,4,7),GF(0,0,7,0,0),GF(0,0,0,0,2),GF(0,2,0,2,0),GF(1,1,2,4,4), /* Z - . : / */
  GF(0,0,0,2,4),GF(2,2,2,0,2),GF(7,1,2,0,2),GF(2,2,0,0,0)                 /* , ! ? ' (서사 §08) */
};
static int glyph_idx(char c){
  if(c>='0'&&c<='9') return c-'0';
  if(c>='A'&&c<='Z') return 10+c-'A';
  if(c=='-') return 36; if(c=='.') return 37; if(c==':') return 38; if(c=='/') return 39;
  if(c==',') return 40; if(c=='!') return 41; if(c=='?') return 42; if(c=='\'') return 43;
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
/* 다줄 교신 렌더 (서사 §08): \n 분할, cut=표시 글자수(타이핑, <0=전체) */
static void draw_text_multi(float x,float y,float s,const char *t,float r,float g,float b,float a,int cut){
  char line[80]; int n=0, shown=0; float ly=y;
  for(;;t++){ char c=*t;
    if(c==0){ line[n]=0; draw_text(x,ly,s,line,r,g,b,a); break; }
    if(c=='\n'){ line[n]=0; draw_text(x,ly,s,line,r,g,b,a); ly+=s*6.5f; n=0; continue; }
    if(cut>=0&&shown>=cut){ line[n]=0; draw_text(x,ly,s,line,r,g,b,a); break; }
    if(n<79) line[n++]=c; shown++;
  }
}
/* ---- 시질 (절차 벡터 엠블럼, §08 §2; 화자/캐릭터 상징) ---- */
static void draw_sigil(float cx,float cy,float s,int type,float r,float g,float b,float a){
  int k; glColor4f(r,g,b,a);
  if(type==0){ /* ECHO: 점멸 삼각 + 노이즈 점 */
    float ps=s*(0.9f+0.12f*f_sin(g_time*7.0f));
    glBegin(GL_LINE_LOOP); glVertex2f(cx,cy-ps); glVertex2f(cx+ps*0.88f,cy+ps*0.62f); glVertex2f(cx-ps*0.88f,cy+ps*0.62f); glEnd();
    glColor4f(0.7f,1.0f,1.0f,a); float nx=cx+fxsym()*2.5f,ny=cy+fxsym()*2.5f;
    glBegin(GL_QUADS); glVertex2f(nx-1.5f,ny-1.5f); glVertex2f(nx+1.5f,ny-1.5f); glVertex2f(nx+1.5f,ny+1.5f); glVertex2f(nx-1.5f,ny+1.5f); glEnd();
  } else if(type==1){ /* REVENANT: 다이아 + 깨진 링 */
    glBegin(GL_LINE_LOOP); glVertex2f(cx,cy-s); glVertex2f(cx+s,cy); glVertex2f(cx,cy+s); glVertex2f(cx-s,cy); glEnd();
    glBegin(GL_LINES); for(k=0;k<5;k++){ float a1=g_time*0.8f+(float)k*1.2566f,a2=a1+0.7f; glVertex2f(cx+f_cos(a1)*s*1.5f,cy+f_sin(a1)*s*1.5f); glVertex2f(cx+f_cos(a2)*s*1.5f,cy+f_sin(a2)*s*1.5f); } glEnd();
  } else if(type==2){ /* DAEMON: 가시 클러스터 */
    glBegin(GL_LINES); for(k=0;k<6;k++){ float aa=(float)k*1.0472f+g_time*1.6f; glVertex2f(cx,cy); glVertex2f(cx+f_cos(aa)*s*1.3f,cy+f_sin(aa)*s*1.3f); } glEnd();
  } else if(type==3){ /* SENTINEL: 육각 이중 방패 */
    glBegin(GL_LINE_LOOP); for(k=0;k<6;k++){ float aa=(float)k*1.0472f+0.52f; glVertex2f(cx+f_cos(aa)*s,cy+f_sin(aa)*s); } glEnd();
    glBegin(GL_LINE_LOOP); for(k=0;k<6;k++){ float aa=(float)k*1.0472f+0.52f; glVertex2f(cx+f_cos(aa)*s*0.6f,cy+f_sin(aa)*s*0.6f); } glEnd();
  } else { /* GHOST: 흐린 이중 링 */
    glColor4f(r,g,b,a*0.6f);
    glBegin(GL_LINE_LOOP); for(k=0;k<14;k++){ float aa=(float)k/14.0f*6.2832f; glVertex2f(cx+f_cos(aa)*s,cy+f_sin(aa)*s); } glEnd();
    glBegin(GL_LINE_LOOP); for(k=0;k<14;k++){ float aa=(float)k/14.0f*6.2832f+g_time*0.5f; glVertex2f(cx+f_cos(aa)*s*0.55f,cy+f_sin(aa)*s*0.55f); } glEnd();
  }
}
/* ---- 서사 교신 (영문 기계 교신체, §08; ECHO=신뢰불가 화자) ---- */
static const char* g_xmitLayer[6]={
  "ECHO: YOU ARE AWAKE.\nTHE COOLANT STILL HOLDS.\nDESCEND. PURGE THE CORE.",
  "ECHO: THE WALLS LISTEN HERE.\nTRUST THE SIGNAL, NOT THE WALLS.",
  "ECHO: COMPUTE LAYER.\nSOMETHING COUNTS YOUR STEPS.",
  "ECHO: MEMORY OPENS.\nIT REMEMBERS WHAT YOU WERE.",
  "ECHO: POWER FLOODS THE GRID.\nDO NOT STOP MOVING.",
  "ECHO: KERNEL. THE CORE IS NEAR.\nSO IS THE TRUTH, REVENANT."
};
static const char* g_xmitBoss[3]={
  "THE CORE: ACCESS DENIED.\nI AM THE FIREWALL. TURN BACK.",
  "THE WARDEN: ARCHIVE 7741.\nYOU WERE DELETED FOR A REASON.\nI AM THAT REASON.",
  "THE NEXUS: WE ARE THE NETWORK.\nWE HAVE WAITED FOR YOU."
};
static const char* g_xmitDeath[4]={
  "THE COOLANT TAKES YOU BACK.",
  "COMPUTE RECALCULATES YOU.",
  "MEMORY REWRITES YOU. THE ROT REMEMBERS.",
  "THE KERNEL KEEPS YOUR ECHO."
};
static const char* g_xmitMsg; static float g_xmitT; static const char* g_deathMsg;
static int narr_zone(int d){ return d<=1?0:(d<=2?1:(d<=3?2:(d<=5?3:(d<=8?4:5)))); }
static void set_xmit(const char*m){ g_xmitMsg=m; g_xmitT=0.0f; }
/* ---- 코덱스 파편 (해금형 lore, §08 N2) — 8종 ---- */
#define CODEXN 8
static const char* g_codexTxt[CODEXN]={
  "FRAG 01: THE MAINFRAME WAS\nBUILT TO DREAM. IT DREAMED OF US.",
  "FRAG 02: THE ROT IS NO INVADER.\nIT IS THE MACHINE, REMEMBERING.",
  "FRAG 03: ARCHIVE 7741 WAS A\nPERSON ONCE. YOU WEAR THEIR NAME.",
  "FRAG 04: ECHO SPEAKS WITH THE\nADMIN'S VOICE. THE ADMIN IS DELETED.",
  "FRAG 05: EACH DESCENT REWRITES\nMORE OF YOU. LESS IS LEFT EACH TIME.",
  "FRAG 06: THE CORE DOES NOT GUARD\nTHE MACHINE. IT GUARDS THE EXIT.",
  "FRAG 07: ECHO IS WHAT IS LEFT\nOF YOU. YOU HAVE DONE THIS BEFORE.",
  "FRAG 08: PURGE. MERGE. ESCAPE.\nONLY ONE OF THESE IS REAL."
};
/* ---- 메타 세이브 (no-CRT Win32, §08 N2; red-team: 환경변수·체크섬·원자적쓰기) ---- */
#define SAVE_MAGIC 0x4E44524Eu
#define SAVE_VER 1u
static unsigned int save_sum(const unsigned int* f,int n){ unsigned int h=2166136261u; int i; for(i=0;i<n;i++){ h^=f[i]; h*=16777619u; } return h; }
static int save_path(char* o,int n){ /* %LOCALAPPDATA%\neondescent.dat (shell32 미링크 → 환경변수) */
  unsigned int len=GetEnvironmentVariableA("LOCALAPPDATA",o,(DWORD)n);
  int i; const char* suf="\\neondescent.dat"; const char* p;
  if(len==0||len>=(unsigned)(n-20)){ o[0]='.'; len=1; }
  i=(int)len; p=suf; while(*p&&i<n-1) o[i++]=*p++; o[i]=0; return i;
}
static void save_load(void){
  char path[300]; unsigned int f[8]; DWORD got=0; HANDLE h;
  g_bestScore=0; g_bestLayer=0; g_corruption=0; g_codex=0; g_bestTime=0; /* 기본값 */
  save_path(path,sizeof(path));
  h=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
  if(h==INVALID_HANDLE_VALUE) return;
  if(ReadFile(h,f,sizeof(f),&got,0)&&got==sizeof(f)&&f[0]==SAVE_MAGIC&&f[1]==SAVE_VER&&f[7]==save_sum(f,7)){
    g_bestScore=(int)f[2]; g_bestLayer=(int)f[3]; g_corruption=(int)f[4]; g_codex=f[5]; g_bestTime=(int)f[6];
  }
  CloseHandle(h);
}
static void save_write(void){
  char path[300], tmp[320]; unsigned int f[8]; DWORD put=0; HANDLE h; int i; const char* e;
  f[0]=SAVE_MAGIC; f[1]=SAVE_VER; f[2]=(unsigned)g_bestScore; f[3]=(unsigned)g_bestLayer;
  f[4]=(unsigned)g_corruption; f[5]=g_codex; f[6]=(unsigned)g_bestTime; f[7]=save_sum(f,7);
  save_path(path,sizeof(path));
  for(i=0;path[i]&&i<300;i++) tmp[i]=path[i]; e=".tmp"; while(*e)tmp[i++]=*e++; tmp[i]=0;
  h=CreateFileA(tmp,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
  if(h==INVALID_HANDLE_VALUE) return;
  WriteFile(h,f,sizeof(f),&put,0); CloseHandle(h);
  if(put==sizeof(f)) MoveFileExA(tmp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH); else DeleteFileA(tmp);
}
static void unlock_codex(int bit){ if(bit>=0&&bit<CODEXN&&!(g_codex&(1u<<bit))){ g_codex|=(1u<<bit); set_xmit(g_codexTxt[bit]); save_write(); } }
/* ---- 페르소나 (Sephiria式 선택 주인공, §08 N3; 해금=세이브 스탯 파생) ---- */
static int g_persona;
static const char* g_persoName[4]={"REVENANT","DAEMON","SENTINEL","GHOST"};
static const char* g_persoDesc[4]={"BALANCED. WHO ERASED YOU?","GLASS CANNON. DELETION IS JOY.","SHIELDED. A MASTERLESS GUARD.","SWIFT. A FAILED UPLOAD."};
static const char* g_persoIntro[4]={0,
  "DAEMON.EXE ONLINE.\nDELETION IS A PLEASURE. BEGIN.",
  "SENTINEL ACTIVE.\nMY MASTER IS GONE. YOU WILL DO.",
  "GHOST. I FAILED TO UPLOAD.\nNOW I HAUNT THE WIRES WITH YOU."};
static int perso_unlocked(int p){
  if(p==0) return 1;
  if(p==1) return g_bestLayer>=5;
  if(p==2) return (g_codex&(1u<<5))||g_bestLayer>=3;
  if(p==3) return g_corruption>=5;
  return 0;
}
/* ---- 엔딩 (§08 N3b; L12 KERNEL 최종 CORE 격파 → 조건 분기) ---- */
#define END_PURGE 0
#define END_MERGE 1
#define END_ESCAPE 2
#define END_ROT 3
static const char* g_endTitle[4]={"PURGE","ASCENSION","ESCAPE","THE ROT"};
static const char* g_endText[4]={
  "THE CORE GOES DARK.\nTHE ROT IS GONE. SO IS EVERYTHING ELSE.\nYOU PURGED THE MACHINE.\nAND THE MACHINE WAS ALL THERE WAS.",
  "YOU DO NOT DESTROY THE CORE.\nYOU REPLACE IT. THE MACHINE IS YOURS.\nYOU REMEMBER EVERYTHING NOW.\nYOU ARE THE NEW DREAM.",
  "THE EXIT OPENS. ECHO WAS RIGHT.\nYOU UPLOAD. THE WIRES FALL AWAY.\nSOMEWHERE A SCREEN GOES QUIET.\nYOU ARE FREE. YOU ARE GONE.",
  "THE CORE LAUGHS. IT WAS NEVER THE ENEMY.\nYOU WERE THE ROT. ALWAYS.\nEACH DESCENT FED YOU. NOW YOU ARE WHOLE.\nTHE MACHINE DREAMS OF YOU NOW."
};
static int g_finalBoss, g_ending; static const char* g_endMsg;
static int compute_ending(void){
  if(g_corruption>=10) return END_ROT;        /* 너무 많이 죽음 → 너가 ROT */
  if(g_persona==3) return END_ESCAPE;          /* GHOST → 업로드 탈출 */
  if(popcnt((unsigned char)g_codex)>=6) return END_MERGE; /* 진실을 거의 다 앎 → 합일 */
  return END_PURGE;
}
static void fmt_int(char *buf,int v){ /* 자작 정수→글리프 (printf 금지, rules/20) */
  char tmp[12]; int n=0,i;
  if(v<0)v=0;
  do{ tmp[n++]='0'+(v%10); v/=10; }while(v&&n<11);
  for(i=0;i<n;i++) buf[i]=tmp[n-1-i];
  buf[n]=0;
}
/* 문자열 이어붙이기 헬퍼: b[n..]에 s 복사, 새 길이 반환. HUD 텍스트 조립 보일러플레이트 축약 */
static int sapp(char*b,int n,const char*s){ while(*s)b[n++]=*s++; return n; }
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

