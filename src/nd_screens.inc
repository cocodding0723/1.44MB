static void render_hud(int w,int h){
  int i;
  hud_begin(w,h);
  draw_hearts();
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
  /* 미니맵 (§18 M6): 우상단, 셀 8px. Tab 확대(스트레치) → 2× */
  { int gx,gy, pt=pal_tier();
    int pgx=(int)(g_player.pos.x/TILEF)/CELLT, pgy=(int)(g_player.pos.y/TILEF)/CELLT;
    float cs=g_mapZoom?16.0f:8.0f, step=g_mapZoom?20.0f:10.0f, ms=cs/8.0f;
    float mx0=(float)w-20.0f-9.0f*step, my0=72.0f;
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
      float bx=mx0+(float)gx*step, by=my0+(float)gy*step;
      if(seen){
        int cur=(gx==pgx&&gy==pgy);
        if(cur) glColor4f(1.0f,1.0f,1.0f,0.95f);
        else glColor4f(g_palAc[pt][0],g_palAc[pt][1],g_palAc[pt][2],0.4f);
        glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+cs,by); glVertex2f(bx+cs,by+cs); glVertex2f(bx,by+cs); glEnd();
      } else {
        glColor4f(g_palAc[pt][0],g_palAc[pt][1],g_palAc[pt][2],0.35f);
        glBegin(GL_LINE_LOOP); glVertex2f(bx,by); glVertex2f(bx+cs,by); glVertex2f(bx+cs,by+cs); glVertex2f(bx,by+cs); glEnd();
      }
      if(seen){ /* 특수 마커 (확대 시 ms 배율) */
        unsigned char rt=g_grid[gy][gx].type;
        if(rt==2&&!g_cleared[gy][gx]) diamond_fill(bx+cs*0.5f,by+cs*0.5f,3.0f*ms,1.0f,0.3f,0.85f,0.95f);
        else if(rt==3){
          if(boss_layer()&&!g_bossDead){ glColor4f(1.0f,0.3f,0.3f,0.95f);
            glBegin(GL_LINES); glVertex2f(bx+1.5f*ms,by+1.5f*ms); glVertex2f(bx+6.5f*ms,by+6.5f*ms); glVertex2f(bx+6.5f*ms,by+1.5f*ms); glVertex2f(bx+1.5f*ms,by+6.5f*ms); glEnd(); }
          else { glColor4f(0.2f,1.0f,0.85f,0.95f);
            glBegin(GL_TRIANGLES); glVertex2f(bx+1.5f*ms,by+2.0f*ms); glVertex2f(bx+6.5f*ms,by+2.0f*ms); glVertex2f(bx+4.0f*ms,by+6.5f*ms); glEnd(); }
        }
        else if(rt==4){ /* 상점 $ — 앰버 마커 (§18) */
          glColor4f(1.0f,0.8f,0.2f,0.95f); diamond_fill(bx+cs*0.5f,by+cs*0.5f,3.0f*ms,1.0f,0.8f,0.2f,0.95f);
          glBegin(GL_LINES); glVertex2f(bx+cs*0.5f,by+1.0f*ms); glVertex2f(bx+cs*0.5f,by+7.0f*ms); glEnd(); }
      }
    }
    if(g_mapZoom) draw_text(mx0,my0-16.0f,1.4f,"MAP - TAB",g_palAc[pt][0],g_palAc[pt][1],g_palAc[pt][2],0.7f);
  }
  draw_hurt_vignette(w,h);
  /* 방 클리어 정화 플래시 (DESCENT 전용) */
  if(g_clearFx>0.0f&&g_optFlash>0.0f){ glColor4f(0.5f,1.0f,0.95f,g_clearFx*0.22f*g_optFlash);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f((float)w,0); glVertex2f((float)w,(float)h); glVertex2f(0,(float)h); glEnd(); }
  draw_comms_overlay(w,h);
  draw_crosshair();
}

/* OVERCLOCK 전용 HUD (docs/07 §7): XP 바 + LV + 타이머 + 킬, 공용 헬퍼 재사용 */
static void oc_render_hud(int w,int h){
  hud_begin(w,h);
  draw_hearts();
  /* XP 바 + LV */
  { float bx=120.0f, by=16.0f, bw=(float)w-300.0f; float f=(float)g_ocXP/(float)(g_ocXPneed>0?g_ocXPneed:1); if(f>1.0f)f=1.0f;
    glColor4f(0.05f,0.14f,0.09f,0.8f); glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+bw,by); glVertex2f(bx+bw,by+10.0f); glVertex2f(bx,by+10.0f); glEnd();
    glColor4f(0.4f,1.0f,0.6f,0.9f); glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+bw*f,by); glVertex2f(bx+bw*f,by+10.0f); glVertex2f(bx,by+10.0f); glEnd();
    glColor4f(0.5f,1.0f,0.7f,0.6f); glBegin(GL_LINE_LOOP); glVertex2f(bx,by); glVertex2f(bx+bw,by); glVertex2f(bx+bw,by+10.0f); glVertex2f(bx,by+10.0f); glEnd();
    char lb[12]; lb[0]='L'; lb[1]='V'; lb[2]=' '; fmt_int(lb+3,g_ocLevel); draw_text(20.0f,46.0f,2.0f,lb,0.5f,1.0f,0.7f,0.95f); }
  /* 타이머 mm:ss (우상) */
  { int sec=(int)g_ocT, mm=sec/60, ss=sec%60; char tb[8]; int n=0;
    tb[n++]=(char)('0'+(mm/10)%10); tb[n++]=(char)('0'+mm%10); tb[n++]=':'; tb[n++]=(char)('0'+ss/10); tb[n++]=(char)('0'+ss%10); tb[n]=0;
    draw_text((float)w-text_w(tb,2.6f)-20.0f,18.0f,2.6f,tb,1.0f,1.0f,1.0f,0.95f); }
  /* KILLS */
  { char kb[16]; int n=0; n=sapp(kb,n,"KILLS "); fmt_int(kb+n,g_kills);
    draw_text((float)w-text_w(kb,1.6f)-20.0f,50.0f,1.6f,kb,0.6f,0.95f,1.0f,0.8f); }
  /* 보스 HP 바 */
  if(g_boss.active){ float bw=380.0f, bx=((float)w-bw)*0.5f, by=34.0f, fr2=g_boss.hp/g_boss.maxhp; if(fr2<0.0f)fr2=0.0f;
    center_text(w,64.0f,1.4f,g_boss.type==0?"SUBCORE - THE CORE":(g_boss.type==2?"SUBCORE - THE NEXUS":"SUBCORE - THE WARDEN"),1.0f,0.4f,0.5f,0.9f);
    glColor4f(0.15f,0.05f,0.1f,0.8f); glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+bw,by); glVertex2f(bx+bw,by+9.0f); glVertex2f(bx,by+9.0f); glEnd();
    glColor4f(1.0f,0.3f,0.45f,0.95f); glBegin(GL_QUADS); glVertex2f(bx,by); glVertex2f(bx+bw*fr2,by); glVertex2f(bx+bw*fr2,by+9.0f); glVertex2f(bx,by+9.0f); glEnd(); }
  draw_hurt_vignette(w,h);
  draw_comms_overlay(w,h);
  draw_crosshair();
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

static void render_codex(int w,int h){ /* 코덱스 뷰어 (§08 N2) — 해금 파편 열람 */
  int i; glViewport(0,0,w,h); glClearColor(0.02f,0.03f,0.05f,1.0f); glClear(GL_COLOR_BUFFER_BIT);
  hud_begin(w,h);
  center_text(w,(float)h*0.05f,3.0f,"CODEX - FRAGMENTS",0.5f,0.95f,1.0f,0.9f);
  float y=(float)h*0.15f;
  for(i=0;i<CODEXN;i++){
    if(g_codex&(1u<<i)){ diamond_fill(46.0f,y+5.0f,4.0f,0.5f,1.0f,0.9f,0.9f); /* 해금 마커 */
      draw_text_multi(60.0f,y,1.6f,g_codexTxt[i],0.6f,1.0f,0.95f,0.92f,-1); }
    else { char line[20]; int n=0; n=sapp(line,n,"FRAG 0"); line[n++]=(char)('1'+i);
      { n=sapp(line,n,": LOCKED"); } line[n]=0;
      draw_text(60.0f,y,1.6f,line,0.4f,0.4f,0.5f,0.55f); }
    y+=(float)h*0.094f;
  }
  center_text(w,(float)h*0.955f,1.6f,"ESC - BACK",0.7f,0.7f,0.8f,0.6f);
}
static void render_ending(int w,int h){ /* 엔딩 화면 (§08 N3b) */
  glViewport(0,0,w,h); glClearColor(0.01f,0.01f,0.02f,1.0f); glClear(GL_COLOR_BUFFER_BIT); hud_begin(w,h);
  float r=0.6f,g=0.9f,b=1.0f;
  if(g_ending==END_ROT){ r=1.0f; g=0.3f; b=0.3f; }
  else if(g_ending==END_MERGE){ r=0.7f; g=0.5f; b=1.0f; }
  else if(g_ending==END_ESCAPE){ r=0.5f; g=1.0f; b=0.8f; }
  center_text(w,(float)h*0.15f,5.0f,g_endTitle[g_ending],r,g,b,0.95f);
  draw_sigil((float)w*0.5f,(float)h*0.27f,24.0f,g_ending+1,r,g,b,0.8f); /* 엔딩별 시질 */
  if(g_endMsg) draw_text_multi((float)w*0.5f-280.0f,(float)h*0.40f,2.0f,g_endMsg,0.85f,0.92f,1.0f,0.85f,-1);
  { float bl=0.5f+0.5f*f_sin(g_time*3.0f);
    center_text(w,(float)h*0.86f,1.8f,"R - DESCEND ANEW    ESC - TITLE",0.8f,0.8f,0.9f,0.4f+0.4f*bl); }
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
  /* 모드 선택 (docs/07): DESCENT / OVERCLOCK */
  center_text(w,cy+92.0f,2.2f, g_mode==MODE_OVERCLOCK?"MODE   OVERCLOCK":"MODE   DESCENT",
    g_mode==MODE_OVERCLOCK?1.0f:0.35f, g_mode==MODE_OVERCLOCK?0.55f:0.95f, g_mode==MODE_OVERCLOCK?0.2f:1.0f, 0.9f);
  center_text(w,cy+114.0f,1.3f,"TAB - SWITCH MODE",0.6f,0.8f,0.95f,0.5f);
  /* 페르소나 선택 (§08 N3) — 고유 시질 + 셀렉터 */
  { int u=perso_unlocked(g_persona); float py=(float)h*0.555f;
    draw_sigil((float)w*0.5f,py-50.0f,15.0f,g_persona+1, u?0.6f:0.4f, u?1.0f:0.45f, u?0.95f:0.5f, u?0.85f:0.55f); /* §2 페르소나 엠블럼 */
    center_text(w,py-22.0f,1.3f,"A - PREV    PERSONA    NEXT - D",0.5f,0.7f,0.95f,0.5f);
    center_text(w,py,3.4f,g_persoName[g_persona], u?0.6f:0.45f, u?1.0f:0.45f, u?0.95f:0.5f, u?0.95f:0.6f);
    center_text(w,py+34.0f,1.4f, u?g_persoDesc[g_persona]:"LOCKED - PROVE YOURSELF DEEPER", 0.6f,0.85f,1.0f, u?0.65f:0.45f); }
  float bl=0.5f+0.5f*f_sin(g_time*3.2f);
  center_text(w,(float)h*0.66f,2.6f, g_mode==MODE_OVERCLOCK?"CLICK / SPACE - OVERCLOCK":"CLICK / SPACE - DESCEND",1.0f,1.0f,1.0f,0.35f+0.5f*bl);
  center_text(w,(float)h*0.70f,1.5f,"WASD MOVE - LMB SHOOT - SPACE DASH - RMB BLINK - Q EMP",0.5f,0.85f,1.0f,0.55f);
  center_text(w,(float)h*0.745f,1.3f, g_mode==MODE_OVERCLOCK?"GOAL: SURVIVE THE SWARM - AUTO-FIRE - LEVEL UP":"GOAL: REACH AND PURGE THE CORE - LAYER 12",0.5f,0.7f,0.9f,0.45f);
  if(g_bestScore>0){ char buf[12];
    float y=(float)h*0.78f;
    center_text(w,y,2.0f,"BEST",0.4f,0.9f,1.0f,0.7f);
    fmt_int(buf,g_bestLayer);
    { char line[24]; int n=0; n=sapp(line,n,"LAYER "); n=sapp(line,n,buf); line[n]=0;
      center_text(w,y+26.0f,2.0f,line,1.0f,1.0f,1.0f,0.8f); }
    fmt_int(buf,g_bestScore);
    { char line[24]; int n=0; n=sapp(line,n,"SCORE "); n=sapp(line,n,buf); line[n]=0;
      center_text(w,y+52.0f,2.0f,line,1.0f,1.0f,1.0f,0.8f); }
  }
  { char buf[12]; char line[36]; int n=0; n=sapp(line,n,"CODEX "); /* 메타: 코덱스/부패도 (§08 N2) */
    fmt_int(buf,popcnt((unsigned char)g_codex)); { n=sapp(line,n,buf); }
    { n=sapp(line,n,"/8   C TO VIEW"); } line[n]=0;
    center_text(w,(float)h*0.90f,1.5f,line,0.5f,0.85f,1.0f,0.55f); }
}

static void render_gameover(int w,int h){
  render_world(w,h);
  hud_begin(w,h);
  dim_screen(w,h,0.68f);
  float cy=(float)h*0.22f;
  center_text(w,cy,5.0f, g_mode==MODE_OVERCLOCK?"OVERCLOCK ENDS":"SIGNAL LOST",1.0f,0.3f,0.4f,0.95f);
  if(g_mode==MODE_OVERCLOCK){ /* OVERCLOCK 정산: 생존시간·LV·킬·점수 (docs/07 §3.7) */
    { int sec=(int)g_ocT, mm=sec/60, ss=sec%60; char line[28]; int n=0; n=sapp(line,n,"SURVIVED ");
      line[n++]=(char)('0'+(mm/10)%10); line[n++]=(char)('0'+mm%10); line[n++]=':'; line[n++]=(char)('0'+ss/10); line[n++]=(char)('0'+ss%10); line[n]=0;
      center_text(w,cy+58.0f,2.4f,line,1.0f,1.0f,1.0f,0.85f); }
    { float y=cy+110.0f; char buf[12]; char line[32]; int n;
      n=0; { n=sapp(line,n,"LEVEL X200 : "); } fmt_int(buf,g_ocLevel*200); { n=sapp(line,n,buf); } line[n]=0;
      center_text(w,y,2.0f,line,0.5f,1.0f,0.7f,0.85f);
      n=0; { n=sapp(line,n,"KILLS X5 : "); } fmt_int(buf,g_kills*5); { n=sapp(line,n,buf); } line[n]=0;
      center_text(w,y+26.0f,2.0f,line,0.6f,0.95f,1.0f,0.85f);
      n=0; { n=sapp(line,n,"BOSS X1000 : "); } fmt_int(buf,g_bossKills*1000); { n=sapp(line,n,buf); } line[n]=0;
      center_text(w,y+52.0f,2.0f,line,1.0f,0.5f,0.8f,0.85f);
      n=0; { n=sapp(line,n,"SCORE "); } fmt_int(buf,oc_score()); { n=sapp(line,n,buf); } line[n]=0;
      center_text(w,y+96.0f,3.2f,line,1.0f,1.0f,1.0f,1.0f);
      n=0; { n=sapp(line,n,"( BEST "); } fmt_int(buf,g_bestScore); { n=sapp(line,n,buf); } { n=sapp(line,n," )"); } line[n]=0;
      center_text(w,y+132.0f,1.8f,line,0.6f,0.9f,1.0f,0.75f);
    }
  } else {
  { char buf[12]; char line[28]; int n=0; n=sapp(line,n,"LAYER ");
    fmt_int(buf,g_depth); { n=sapp(line,n,buf); }
    { n=sapp(line,n," REACHED"); } line[n]=0;
    center_text(w,cy+58.0f,2.4f,line,1.0f,1.0f,1.0f,0.85f); }
  if(g_deathMsg) center_text(w,cy+90.0f,1.6f,g_deathMsg,0.75f,0.5f,0.55f,0.8f); /* 서사: 사망 에피타프 */
  /* 점수 분해 (§17) */
  { float y=cy+120.0f; char buf[12]; char line[32]; int n;
    n=0; { n=sapp(line,n,"LAYER X1000 : "); } fmt_int(buf,g_depth*1000); { n=sapp(line,n,buf); } line[n]=0;
    center_text(w,y,2.0f,line,0.6f,0.95f,1.0f,0.85f);
    n=0; { n=sapp(line,n,"KILLS X25 : "); } fmt_int(buf,g_kills*25); { n=sapp(line,n,buf); } line[n]=0;
    center_text(w,y+26.0f,2.0f,line,0.6f,0.95f,1.0f,0.85f);
    n=0; { n=sapp(line,n,"BITS X5 : "); } fmt_int(buf,g_bits*5); { n=sapp(line,n,buf); } line[n]=0;
    center_text(w,y+52.0f,2.0f,line,1.0f,0.95f,0.5f,0.85f);
    n=0; { n=sapp(line,n,"BOSS X500 : "); } fmt_int(buf,g_bossKills*500); { n=sapp(line,n,buf); } line[n]=0;
    center_text(w,y+78.0f,2.0f,line,1.0f,0.5f,0.8f,0.85f);
    n=0; { n=sapp(line,n,"SCORE "); } fmt_int(buf,score_now()); { n=sapp(line,n,buf); } line[n]=0;
    center_text(w,y+122.0f,3.2f,line,1.0f,1.0f,1.0f,1.0f);
    n=0; { n=sapp(line,n,"( BEST "); } fmt_int(buf,g_bestScore); { n=sapp(line,n,buf); } { n=sapp(line,n," )"); } line[n]=0;
    center_text(w,y+158.0f,1.8f,line,0.6f,0.9f,1.0f,0.75f); /* §18 목업 BEST 줄 */
  }
  }
  float bl=0.5f+0.5f*f_sin(g_time*3.2f);
  center_text(w,(float)h*0.82f,2.4f, g_mode==MODE_OVERCLOCK?"R - OVERCLOCK AGAIN":"R - DESCEND AGAIN",1.0f,1.0f,1.0f,0.35f+0.5f*bl);
  center_text(w,(float)h*0.82f+30.0f,1.6f,"ESC - TITLE",0.7f,0.7f,0.8f,0.6f);
}

static void render_pause(int w,int h){
  render_world(w,h);
  if(g_mode==MODE_OVERCLOCK) oc_render_hud(w,h); else render_hud(w,h);
  hud_begin(w,h);
  dim_screen(w,h,0.55f);
  center_text(w,(float)h*0.38f,5.0f,"PAUSED",0.55f,1.0f,1.0f,0.95f);
  center_text(w,(float)h*0.55f,2.0f,"ESC RESUME - Q QUIT",1.0f,1.0f,1.0f,0.6f);
  /* 옵션 (§18 광과민 대응): S 셰이크 / F 플래시 / M 음소거 */
  { char line[80]; int n=0; n=sapp(line,n,"S SHAKE ");
    { char b2[8]; fmt_int(b2,(int)(g_optShake*100.0f)); n=sapp(line,n,b2); }
    { n=sapp(line,n,"  F FLASH "); }
    { char b2[8]; fmt_int(b2,(int)(g_optFlash*100.0f)); n=sapp(line,n,b2); }
    { n=sapp(line,n,"  C CRT "); }
    { char b2[8]; fmt_int(b2,(int)(g_optCRT*100.0f)); n=sapp(line,n,b2); }
    { n=sapp(line,n,"  M MUTE "); }
    { n=sapp(line,n,g_mute?"ON":"OFF"); }
    { n=sapp(line,n,"  B BGM "); }
    { n=sapp(line,n,g_bgm?"ON":"OFF"); }
    line[n]=0;
    center_text(w,(float)h*0.63f,1.6f,line,0.6f,0.9f,1.0f,0.6f);
  }
  /* OVERCLOCK 빌드 요약 (docs/07 §7): 보유 무기/레벨·진화 */
  if(g_mode==MODE_OVERCLOCK){
    center_text(w,(float)h*0.72f,1.6f,"- ARSENAL -",0.5f,1.0f,0.7f,0.7f);
    int wi, shown=0;
    for(wi=0;wi<WEAPN;wi++){ if(!g_weapLvl[wi]) continue;
      char line[40]; int n=0; n=sapp(line,n,g_weapMeta[wi].name); line[n++]=' ';
      if(g_weapEvo[wi]){ n=sapp(line,n,"*EVO*"); line[n]=0; }
      else { line[n++]='L'; line[n++]='V'; line[n++]=' '; char nb[8]; fmt_int(nb,g_weapLvl[wi]); n=sapp(line,n,nb); line[n]=0; }
      center_text(w,(float)h*0.75f+(float)shown*20.0f,1.4f,line, g_weapEvo[wi]?1.0f:0.5f,1.0f,g_weapEvo[wi]?0.4f:0.7f,0.85f);
      shown++;
    }
  }
}

static void render_upgrade(int w,int h){
  int k;
  render_world(w,h);
  hud_begin(w,h);
  dim_screen(w,h,0.6f);
  center_text(w,(float)h*0.18f,3.4f, g_upgCtx==1?"- LEVEL UP -":(g_upgRare?"- RARE MODULE -":"- INSTALL MODULE -"),1.0f,0.4f,0.9f,0.95f);
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
    if(g_upgCtx==1) glColor4f(0.4f,1.0f,0.6f,hov?1.0f:0.55f);
    else if(g_upgRare) glColor4f(1.0f,0.75f,0.25f,hov?1.0f:0.55f);
    else glColor4f(0.3f,0.9f,1.0f,hov?1.0f:0.5f);
    glBegin(GL_LINE_LOOP); glVertex2f(x,y0); glVertex2f(x+cw,y0); glVertex2f(x+cw,y0+ch); glVertex2f(x,y0+ch); glEnd();
    if(m>=0){
      char num[2]; num[0]=(char)('1'+k); num[1]=0;
      draw_text(x+10.0f,y0+10.0f,2.0f,num,0.6f,0.95f,1.0f,0.8f);
      const char *nm, *ds; int lvl;
      if(m>=100){ int wid=m-100; nm=g_weapMeta[wid].name; ds=g_weapMeta[wid].desc; lvl=g_weapLvl[wid]; } /* OC 무기 카드 */
      else { nm=g_modName[m]; ds=g_modDesc[m]; lvl=g_mod[m]; }
      draw_text(x+(cw-text_w(nm,2.0f))*0.5f,y0+52.0f,2.0f,nm,1.0f,1.0f,1.0f,1.0f);
      draw_text(x+(cw-text_w(ds,1.4f))*0.5f,y0+92.0f,1.4f,ds,0.6f,0.95f,1.0f,0.85f);
      if(lvl>0){ char sb[10]; sb[0]='L'; sb[1]='V'; sb[2]=' '; fmt_int(sb+3,lvl+1);
        draw_text(x+(cw-text_w(sb,1.4f))*0.5f,y0+120.0f,1.4f,sb,1.0f,0.75f,0.3f,0.8f); }
    }
  }
  center_text(w,y0+ch+44.0f,1.8f,"CLICK OR 1 - 2 - 3",1.0f,1.0f,1.0f,0.5f);
}

