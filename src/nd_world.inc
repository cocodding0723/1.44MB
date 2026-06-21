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

  /* 상점방 배치 (§13): 비보스 레이어 35% — 전투방 1개를 상점(type 4)으로 대체 */
  g_shopGX=-1; g_shopGY=-1;
  if(!boss_layer() && (xrnd()%100)<35){
    int sgx=-1, sgy=-1, cnt=0;
    for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) if(g_grid[gy][gx].placed && g_grid[gy][gx].type==1){
      cnt++; if((int)(xrnd()%(unsigned)cnt)==0){ sgx=gx; sgy=gy; } } /* 저수지 추첨 */
    if(sgx>=0){ g_grid[sgy][sgx].type=4; g_shopGX=sgx; g_shopGY=sgy; g_cleared[sgy][sgx]=1;
      int base[3]={60,45,30}, q; float m=1.0f+0.1f*(float)(g_depth-1); /* 가격 레이어 비례 +10%/L */
      for(q=0;q<3;q++){ g_shopPrice[q]=(int)((float)base[q]*m); g_shopBought[q]=0; } }
  }

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
/* ---- OVERCLOCK 아레나 생성 (docs/07 §3.2) — 단일 큰 방 + 기둥 엄폐물 ---- */
static void oc_arena_init(void){
  int x,y,p;
  memset(g_tiles,0,sizeof(g_tiles)); memset(g_grid,0,sizeof(g_grid)); memset(g_conn,0,sizeof(g_conn));
  memset(g_cleared,0,sizeof(g_cleared)); memset(g_seen,0,sizeof(g_seen));
  memset(g_ene,0,sizeof(g_ene)); memset(g_bul,0,sizeof(g_bul)); memset(g_ebul,0,sizeof(g_ebul));
  memset(g_part,0,sizeof(g_part)); memset(g_pick,0,sizeof(g_pick)); memset(g_ring,0,sizeof(g_ring));
  memset(g_after,0,sizeof(g_after)); memset(g_boom,0,sizeof(g_boom)); memset(g_trail,0,sizeof(g_trail));
  g_boss.active=0; g_bossDead=0; g_locked=0; g_fireCd=0.0f; g_shopGX=-1; g_shopGY=-1;
  int aw=46, ah=32, ox=(MAPW-aw)/2, oy=(MAPH-ah)/2;
  for(y=oy;y<oy+ah;y++) for(x=ox;x<ox+aw;x++) g_tiles[y][x]=1;
  for(p=0;p<8;p++){ int px=ox+4+(int)(xrnd()%(unsigned)(aw-8)), py=oy+4+(int)(xrnd()%(unsigned)(ah-8));
    if(px>ox+aw/2-3&&px<ox+aw/2+3&&py>oy+ah/2-3&&py<oy+ah/2+3) continue; /* 중앙 스폰 비움 */
    g_tiles[py][px]=3; }
  g_arenaX0=(float)ox*TILEF; g_arenaY0=(float)oy*TILEF; g_arenaX1=(float)(ox+aw)*TILEF; g_arenaY1=(float)(oy+ah)*TILEF;
  g_player.pos.x=((float)ox+(float)aw*0.5f)*TILEF; g_player.pos.y=((float)oy+(float)ah*0.5f)*TILEF;
  g_player.vel.x=0; g_player.vel.y=0; g_cam=g_player.pos;
}
static void descend(void){ g_depth++; g_rng=g_master^((unsigned)g_depth*2654435761u); if(!g_rng)g_rng=1; (void)xrnd(); (void)xrnd(); generate(); set_xmit(g_xmitLayer[narr_zone(g_depth)]);
  if(g_depth>=2)unlock_codex(0); if(g_depth>=4)unlock_codex(1); if(g_depth>=6)unlock_codex(4); if(g_depth>=9)unlock_codex(7); } /* 코덱스: 깊이 마일스톤 */
static void new_run(void){
  LARGE_INTEGER li; QueryPerformanceCounter(&li); g_master=(unsigned int)li.QuadPart|1u;
#ifdef ND_FIXED_SEED
  g_master=(unsigned int)(ND_FIXED_SEED)|1u;                 /* CI/QA 결정론 빌드 (docs/07 §14.1) */
#else
  { char sb[16]; DWORD sl=GetEnvironmentVariableA("ND_SEED",sb,sizeof(sb)); /* 런타임 QA: set ND_SEED=12345 */
    if(sl>0&&sl<16){ unsigned int v=0,i; for(i=0;i<sl;i++){ if(sb[i]<'0'||sb[i]>'9')break; v=v*10u+(unsigned)(sb[i]-'0'); } if(v) g_master=v|1u; } }
#endif
  g_depth=1; g_rng=g_master^2654435761u; if(!g_rng)g_rng=1; (void)xrnd();
  g_pHP=g_pMaxHP=6.0f; g_pIfr=0; g_kills=0; g_bits=0; g_bossKills=0;
  g_dashT=0; g_dashCd=0; g_trauma=0; g_hitstop=0; g_hurtFx=0; g_clearFx=0; g_fringeFx=0; g_time=0;
  g_blinkCd=0; g_empCd=0; g_rmbPressed=0; g_wantDash=0; g_wantEmp=0;
  memset(g_mod,0,sizeof(g_mod));
  g_shotCount=0; g_leechKills=0; g_shieldUp=0;
  g_regenT=0; g_shieldT=0; g_thornT=0; g_slowT=0; g_slowAcc=0; g_frenzy=0; g_siphonK=0;
  g_upgCtx=0; g_bgmIntense=0; g_mapZoom=0; /* QA: 런 간 상태 누수 방지 (BGM 격화/미니맵 줌) */
  /* 페르소나 시작 빌드 (§08 N3) */
  if(g_persona==1) g_mod[M_GLASSCANNON]++;                       /* DAEMON: 글래스캐논 */
  else if(g_persona==2){ g_mod[M_AEGIS]++; g_shieldUp=1; }       /* SENTINEL: 수호막 즉시 */
  else if(g_persona==3){ g_mod[M_AGI]+=2; g_mod[M_COOL]++; }     /* GHOST: 기동 */
  if(g_mode==MODE_OVERCLOCK){ /* OVERCLOCK: 아레나 + XP/레벨/호드 리셋 (docs/07 §3.1) */
    int wi; for(wi=0;wi<WEAPN;wi++){ g_weapLvl[wi]=0; g_weapTimer[wi]=0.0f; g_weapEvo[wi]=0; }
    g_weapLvl[W_PULSE]=1; g_orbAng=0.0f; g_beamAng=0.0f;
    g_ocLevel=1; g_ocXP=0; g_ocXPneed=10; g_ocT=0.0f; g_ocSpawnT=2.0f; g_ocBossT=120.0f; g_ocBossCount=0;
    g_deathMsg=0; g_finalBoss=0; /* 방어적 리셋 (review #2) */
    oc_arena_init();
    set_xmit("OVERCLOCK ENGAGED.\nHOLD THE LINE. THE MACHINE SWARMS.\nLEVEL UP. BURN THEM DOWN.");
    g_state=ST_PLAY;
    return;
  }
  generate();
  set_xmit(g_xmitLayer[0]); g_deathMsg=0; /* 서사: 1레이어 교신 */
  if(g_corruption>=8) set_xmit("ECHO: YOU ARE AWAKE. AGAIN.\nHOW MANY TIMES NOW, REVENANT?\nYOU KNOW THE WAY DOWN.");
  else if(g_corruption>=3) set_xmit("ECHO: YOU ARE AWAKE.\nTHE COOLANT REMEMBERS YOU NOW.\nDESCEND.");
  if(g_persoIntro[g_persona]) set_xmit(g_persoIntro[g_persona]); /* 페르소나 인트로 우선 */
  if(g_corruption>=3)unlock_codex(3); if(g_corruption>=8)unlock_codex(6); /* 코덱스: 부패도 임계 */
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

static void shop_ped(int i,float*px,float*py){ /* 상점 제단 i(0~2)의 월드 좌표 */
  Cell*c=&g_grid[g_shopGY][g_shopGX];
  float fx[3]={0.3f,0.5f,0.7f};
  *px=((float)c->rx+(float)c->rw*fx[i])*TILEF;
  *py=((float)c->ry+(float)c->rh*0.5f)*TILEF;
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
  g_upgCtx=0; /* DESCENT 모듈 적용 → 컨텍스트 확정 (review #3) */
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
    g_hitstop=0.2f; add_trauma(0.8f);
    burst(g_player.pos.x,g_player.pos.y,32,320.0f,1.0f,0.9f,0.9f);
    spawn_ring(g_player.pos.x,g_player.pos.y,10.0f,420.0f,0.5f,1.0f,0.4f,0.5f);
    if(g_mode==MODE_OVERCLOCK){ /* OVERCLOCK: 최고 생존시간 + OC 점수 (docs/07 §3.7) */
      int t=(int)g_ocT; if(t>g_bestTime)g_bestTime=t;
      int sc=oc_score(); if(sc>g_bestScore)g_bestScore=sc; g_deathMsg=0;
    } else {
      int sc=score_now(); if(sc>g_bestScore)g_bestScore=sc;
      if(g_depth>g_bestLayer) g_bestLayer=g_depth;
      { int d=g_depth; g_deathMsg=g_xmitDeath[d<=2?0:(d<=4?1:(d<=7?2:3))]; } /* 서사: 사망 에피타프 */
      g_corruption++; /* 메타: 부패도↑ (DESCENT 전용) */
    }
    save_write();
    g_state=ST_OVER;
  }
}

static void spawn_ebul(float x,float y,float ux,float uy,float spd,float life){
  int i; for(i=0;i<MAXEBUL;i++) if(!g_ebul[i].active){ Bullet*b=&g_ebul[i];
    b->active=1; b->pos.x=x; b->pos.y=y; b->vel.x=ux*spd; b->vel.y=uy*spd; b->life=life; return; }
}
/* 보스 경계/스폰 추출 (docs/07 §14.3) — DESCENT 셀·OVERCLOCK 아레나 양쪽에서 호출 */
