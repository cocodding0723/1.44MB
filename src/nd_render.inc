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

static void ene_color(int t,float*r,float*g,float*b){ /* draw_enemy 색 일치 */
  if(t==0){*r=1.0f;*g=0.30f;*b=0.45f;}
  else if(t==1){*r=0.30f;*g=0.75f;*b=1.0f;}
  else if(t==2){*r=1.0f;*g=0.80f;*b=0.20f;}
  else if(t==5){*r=1.0f;*g=0.45f;*b=0.55f;}
  else if(t==6){*r=0.65f;*g=0.45f;*b=1.0f;}
  else {*r=0.45f;*g=1.0f;*b=0.40f;}
}
/* OVERCLOCK 256적 배칭 렌더 (docs/07 §14.7, 컨펌 oc-batch-v1): 패스당 단일 glBegin, 적별 디테일 단순화 */
static void draw_enemy_batched(void){
  int j; float cr,cg,cb;
  glBegin(GL_QUADS); /* (1) 글로우 1배치 */
  for(j=0;j<MAXENE;j++){ Enemy*e=&g_ene[j]; if(!e->active||e->spawn>0.0f) continue;
    float r=ene_radius(e->type)*1.5f, x=e->pos.x, y=e->pos.y; ene_color(e->type,&cr,&cg,&cb);
    glColor4f(cr,cg,cb,0.20f); glVertex2f(x-r,y-r); glVertex2f(x+r,y-r); glVertex2f(x+r,y+r); glVertex2f(x-r,y+r); }
  glEnd();
  glBegin(GL_TRIANGLES); /* (2) 삼각 코어: 헌터/샤드/랜서 */
  for(j=0;j<MAXENE;j++){ Enemy*e=&g_ene[j]; if(!e->active||e->spawn>0.0f) continue;
    if(e->type!=0&&e->type!=4&&e->type!=5) continue;
    float r=ene_radius(e->type), x=e->pos.x, y=e->pos.y;
    if(e->flash>0.0f){cr=cg=cb=1.0f;} else ene_color(e->type,&cr,&cg,&cb); glColor3f(cr,cg,cb);
    glVertex2f(x,y-r); glVertex2f(x+r*0.9f,y+r*0.7f); glVertex2f(x-r*0.9f,y+r*0.7f); }
  glEnd();
  glBegin(GL_QUADS); /* (3) 사각/다이아 코어: 터릿(사각)/리코셰·포크·위버(다이아) */
  for(j=0;j<MAXENE;j++){ Enemy*e=&g_ene[j]; if(!e->active||e->spawn>0.0f) continue;
    if(e->type==0||e->type==4||e->type==5) continue;
    float r=ene_radius(e->type), x=e->pos.x, y=e->pos.y;
    if(e->flash>0.0f){cr=cg=cb=1.0f;} else ene_color(e->type,&cr,&cg,&cb); glColor3f(cr,cg,cb);
    if(e->type==1){ glVertex2f(x-r,y-r); glVertex2f(x+r,y-r); glVertex2f(x+r,y+r); glVertex2f(x-r,y+r); }
    else { glVertex2f(x,y-r); glVertex2f(x+r,y); glVertex2f(x,y+r); glVertex2f(x-r,y); } }
  glEnd();
  for(j=0;j<MAXENE;j++){ Enemy*e=&g_ene[j]; if(!e->active) continue; /* (4) 엘리트/스폰 소수 개별 */
    if(e->spawn>0.0f){ float r=ene_radius(e->type); ene_color(e->type,&cr,&cg,&cb); glColor4f(cr,cg,cb,0.7f);
      circle_line(e->pos.x,e->pos.y,r+22.0f*(e->spawn/0.4f),12); }
    else if(e->affix){ ene_color(e->type,&cr,&cg,&cb); glColor4f(cr,cg,cb,0.85f); circle_line(e->pos.x,e->pos.y,ene_radius(e->type)+5.0f,10);
      if(e->eshield){ glColor4f(0.5f,0.95f,1.0f,0.6f); circle_line(e->pos.x,e->pos.y,ene_radius(e->type)+9.0f,10); } } }
}
static void hud_begin(int w,int h){
  glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,w,h,0,-1,1); glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}
/* ---- 공용 HUD 헬퍼 (DESCENT·OVERCLOCK 공유 — 중복 제거) ---- */
static void draw_hearts(void){ /* 하트(1하트=2HP, 절반=1HP) */
  int i; for(i=0;i<3;i++){
    float bx=20.0f+i*30.0f, by=24.0f; int hp2=(int)g_pHP-i*2;
    float cr=1.0f,cg=0.25f,cb=0.4f;
    if(hp2>=2) diamond_fill(bx+9.0f,by,11.0f,cr,cg,cb,1.0f);
    else if(hp2==1){ glColor4f(cr,cg,cb,1.0f); glBegin(GL_TRIANGLES); glVertex2f(bx+9.0f,by-11.0f); glVertex2f(bx+9.0f,by+11.0f); glVertex2f(bx-2.0f,by); glEnd();
      glColor4f(0.25f,0.12f,0.16f,1.0f); glBegin(GL_TRIANGLES); glVertex2f(bx+9.0f,by-11.0f); glVertex2f(bx+20.0f,by); glVertex2f(bx+9.0f,by+11.0f); glEnd(); }
    else diamond_fill(bx+9.0f,by,11.0f,0.25f,0.12f,0.16f,1.0f);
  }
}
static void draw_hurt_vignette(int w,int h){ /* 피격 적색 비네트 (광과민 게이트) */
  if(!(g_hurtFx>0.0f&&g_optFlash>0.0f)) return;
  float a=g_hurtFx*0.4f*g_optFlash; float bw=(float)w*0.18f, bh=(float)h*0.22f;
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
static void draw_comms_overlay(int w,int h){ /* 서사 교신 비차단 오버레이 (§08) */
  (void)w;
  if(!(g_xmitMsg && g_xmitT<6.5f)) return;
  float a=1.0f; if(g_xmitT<0.3f)a=g_xmitT/0.3f; else if(g_xmitT>5.5f)a=6.5f-g_xmitT; if(a>1.0f)a=1.0f;
  int cut=(int)(g_xmitT*30.0f); float by=(float)h-98.0f;
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(0.0f,0.02f,0.05f,0.80f*a);
  glBegin(GL_QUADS); glVertex2f(26.0f,by-12.0f); glVertex2f(616.0f,by-12.0f); glVertex2f(616.0f,by+58.0f); glVertex2f(26.0f,by+58.0f); glEnd();
  glColor4f(0.3f,0.9f,1.0f,0.22f*a);
  glBegin(GL_QUADS); glVertex2f(26.0f,by-12.0f); glVertex2f(29.0f,by-12.0f); glVertex2f(29.0f,by+58.0f); glVertex2f(26.0f,by+58.0f); glEnd();
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);
  draw_sigil(52.0f,by+22.0f,11.0f,0,0.3f,0.9f,1.0f,0.9f*a);
  draw_text_multi(74.0f,by,2.0f,g_xmitMsg,0.55f,0.95f,1.0f,0.9f*a,cut);
}
static void draw_crosshair(void){ /* 커스텀 네온 십자선 (스크린 좌표) */
  float mx=(float)g_mouseX, my=(float)g_mouseY;
  glColor3f(1.0f,0.3f,0.6f); glBegin(GL_LINES);
  glVertex2f(mx-9.0f,my); glVertex2f(mx-3.0f,my); glVertex2f(mx+3.0f,my); glVertex2f(mx+9.0f,my);
  glVertex2f(mx,my-9.0f); glVertex2f(mx,my-3.0f); glVertex2f(mx,my+3.0f); glVertex2f(mx,my+9.0f); glEnd();
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

  /* 다운링크/제단 (펄스) — DESCENT 전용 (OVERCLOCK 아레나엔 없음) */
  if(g_mode==MODE_DESCENT&&(!boss_layer()||g_bossDead)){
    float dcx=(g_grid[g_downGY][g_downGX].rx+g_grid[g_downGY][g_downGX].rw*0.5f)*TILEF, dcy=(g_grid[g_downGY][g_downGX].ry+g_grid[g_downGY][g_downGX].rh*0.5f)*TILEF;
    float dpul=1.0f+0.15f*f_sin(g_time*4.0f);
    glColor3f(0.2f,1.0f,0.85f); diamond_line(dcx,dcy,26.0f*dpul); diamond_line(dcx,dcy,16.0f*dpul);
  }
  if(g_mode==MODE_DESCENT){ int gx,gy; for(gy=0;gy<GH;gy++) for(gx=0;gx<GW;gx++) if(g_grid[gy][gx].placed&&g_grid[gy][gx].type==2){
      float mx=(g_grid[gy][gx].rx+g_grid[gy][gx].rw*0.5f)*TILEF, my=(g_grid[gy][gx].ry+g_grid[gy][gx].rh*0.5f)*TILEF;
      if(!g_cleared[gy][gx]){ /* 미사용 제단: 펄스 */
        float ap=1.0f+0.12f*f_sin(g_time*5.0f);
        glColor3f(1.0f,0.3f,0.85f); diamond_line(mx,my,20.0f*ap); diamond_line(mx,my,11.0f*ap);
        diamond_fill(mx,my,6.0f,1.0f,0.4f,0.9f,0.5f+0.3f*f_sin(g_time*5.0f));
      } else { glColor4f(1.0f,0.3f,0.85f,0.25f); diamond_line(mx,my,20.0f); } } }
  /* 상점 제단 3종 (§13) — 네온 다이아 + 이름/가격 월드 라벨 */
  if(g_mode==MODE_DESCENT && g_shopGX>=0){
    static const char* snm[3]={"MODULE","REPAIR","REROLL"};
    int si; for(si=0;si<3;si++){ float spx,spy; shop_ped(si,&spx,&spy);
      float cr=si==0?0.3f:1.0f, cg=si==0?0.9f:(si==1?0.3f:0.8f), cb=si==0?1.0f:(si==1?0.6f:0.2f);
      if(g_shopBought[si]){ glColor4f(cr,cg,cb,0.2f); diamond_line(spx,spy,18.0f); }
      else { float pl=1.0f+0.12f*f_sin(g_time*5.0f+(float)si);
        glColor3f(cr,cg,cb); diamond_line(spx,spy,22.0f*pl); diamond_line(spx,spy,13.0f*pl);
        diamond_fill(spx,spy,7.0f,cr,cg,cb,0.6f);
        draw_text(spx-text_w(snm[si],6.0f)*0.5f,spy-62.0f,6.0f,snm[si],cr,cg,cb,0.9f);
        { char pb[8]; fmt_int(pb,g_shopPrice[si]); draw_text(spx-text_w(pb,6.0f)*0.5f,spy-36.0f,6.0f,pb,1.0f,0.95f,0.3f,0.95f); } }
    }
  }

  /* 픽업 (BITS / 모듈 오브) */
  for(i=0;i<MAXPICK;i++){ if(!g_pick[i].active) continue; Pickup*p=&g_pick[i];
    float pl=0.7f+0.3f*f_sin(p->t*9.0f);
    if(p->kind==1){
      diamond_fill(p->pos.x,p->pos.y,12.0f,1.0f,0.3f,0.9f,0.3f);
      diamond_fill(p->pos.x,p->pos.y,7.0f,1.0f,0.5f,1.0f,pl);
      glColor4f(1.0f,0.4f,0.95f,0.8f); circle_line(p->pos.x,p->pos.y,14.0f+2.0f*f_sin(p->t*6.0f),12);
    } else if(p->kind==2){ /* OVERCLOCK XP 조각 (틸 다이아) */
      diamond_fill(p->pos.x,p->pos.y,8.0f,0.3f,1.0f,0.6f,0.3f);
      diamond_fill(p->pos.x,p->pos.y,4.0f,0.6f,1.0f,0.8f,pl);
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

  /* 적 — OVERCLOCK은 배칭 렌더(256적 60fps, §14.7), DESCENT는 다층 스프라이트 */
  if(g_mode==MODE_OVERCLOCK) draw_enemy_batched();
  else for(i=0;i<MAXENE;i++) if(g_ene[i].active) draw_enemy(&g_ene[i]);

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
        float rcx=g_bossCx, rcy=g_bossCy;
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
  /* OVERCLOCK AURA 장판 시각화 (§4) */
  if(g_mode==MODE_OVERCLOCK&&g_weapLvl[W_AURA]){ int lv=g_weapLvl[W_AURA]; float R=80.0f+18.0f*(float)lv;
    circle_fill(g_player.pos.x,g_player.pos.y,R,28,0.6f,0.4f,1.0f,0.07f+0.03f*f_sin(g_time*4.0f));
    glColor4f(0.7f,0.5f,1.0f,0.4f); circle_line(g_player.pos.x,g_player.pos.y,R,28); }
  /* OVERCLOCK ORBITERS 무기 시각화 (§4) */
  if(g_mode==MODE_OVERCLOCK&&g_weapLvl[W_ORBIT]){ int lv=g_weapLvl[W_ORBIT], n=2+lv, k2; float R=70.0f+10.0f*(float)lv;
    glColor4f(0.4f,0.95f,1.0f,0.25f); circle_line(g_player.pos.x,g_player.pos.y,R,28);
    for(k2=0;k2<n;k2++){ float a=g_orbAng+(float)k2*6.2831853f/(float)n;
      float ox=g_player.pos.x+f_cos(a)*R, oy=g_player.pos.y+f_sin(a)*R;
      diamond_fill(ox,oy,11.0f,0.4f,0.95f,1.0f,0.3f); diamond_fill(ox,oy,6.0f,0.7f,1.0f,1.0f,0.9f); } }
  /* OVERCLOCK BEAM 스윕 레이저 시각화 */
  if(g_mode==MODE_OVERCLOCK&&g_weapLvl[W_BEAM]){ int lv=g_weapLvl[W_BEAM], narm=1+lv/3+(g_weapEvo[W_BEAM]?1:0), k2; if(narm>4)narm=4; float len=240.0f+30.0f*(float)lv+(g_weapEvo[W_BEAM]?60.0f:0.0f);
    for(k2=0;k2<narm;k2++){ float a=g_beamAng+(float)k2*6.2831853f/(float)narm, ux=f_cos(a), uy=f_sin(a);
      draw_beam(g_player.pos.x,g_player.pos.y,ux,uy,len,9.0f,1.0f,0.3f,0.5f,0.22f);
      draw_beam(g_player.pos.x,g_player.pos.y,ux,uy,len,4.0f,1.0f,0.7f,0.9f,0.85f); } }
  /* OVERCLOCK DRONES 시각화 */
  if(g_mode==MODE_OVERCLOCK&&g_weapLvl[W_DRONE]){ int lv=g_weapLvl[W_DRONE], n=1+lv/2+(g_weapEvo[W_DRONE]?1:0), k2; if(n>5)n=5; float R=46.0f;
    for(k2=0;k2<n;k2++){ float a=g_orbAng*0.7f+(float)k2*6.2831853f/(float)n; float ox=g_player.pos.x+f_cos(a)*R, oy=g_player.pos.y+f_sin(a)*R;
      diamond_fill(ox,oy,8.0f,1.0f,0.9f,0.4f,0.3f); diamond_fill(ox,oy,4.0f,1.0f,1.0f,0.7f,0.95f); } }
}

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
  { char kb[16]; int n=0; const char*s="KILLS "; while(*s)kb[n++]=*s++; fmt_int(kb+n,g_kills);
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
    else { char line[20]; int n=0; const char*s="FRAG 0"; while(*s)line[n++]=*s++; line[n++]=(char)('1'+i);
      { const char*s2=": LOCKED"; while(*s2)line[n++]=*s2++; } line[n]=0;
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
    { char line[24]; int n=0; const char*s1="LAYER "; while(*s1)line[n++]=*s1++; const char*p=buf; while(*p)line[n++]=*p++; line[n]=0;
      center_text(w,y+26.0f,2.0f,line,1.0f,1.0f,1.0f,0.8f); }
    fmt_int(buf,g_bestScore);
    { char line[24]; int n=0; const char*s2="SCORE "; while(*s2)line[n++]=*s2++; const char*p=buf; while(*p)line[n++]=*p++; line[n]=0;
      center_text(w,y+52.0f,2.0f,line,1.0f,1.0f,1.0f,0.8f); }
  }
  { char buf[12]; char line[36]; int n=0; const char*s="CODEX "; while(*s)line[n++]=*s++; /* 메타: 코덱스/부패도 (§08 N2) */
    fmt_int(buf,popcnt((unsigned char)g_codex)); { const char*p=buf; while(*p)line[n++]=*p++; }
    { const char*s2="/8   C TO VIEW"; while(*s2)line[n++]=*s2++; } line[n]=0;
    center_text(w,(float)h*0.90f,1.5f,line,0.5f,0.85f,1.0f,0.55f); }
}

static void render_gameover(int w,int h){
  render_world(w,h);
  hud_begin(w,h);
  dim_screen(w,h,0.68f);
  float cy=(float)h*0.22f;
  center_text(w,cy,5.0f, g_mode==MODE_OVERCLOCK?"OVERCLOCK ENDS":"SIGNAL LOST",1.0f,0.3f,0.4f,0.95f);
  if(g_mode==MODE_OVERCLOCK){ /* OVERCLOCK 정산: 생존시간·LV·킬·점수 (docs/07 §3.7) */
    { int sec=(int)g_ocT, mm=sec/60, ss=sec%60; char line[28]; int n=0; const char*s="SURVIVED "; while(*s)line[n++]=*s++;
      line[n++]=(char)('0'+(mm/10)%10); line[n++]=(char)('0'+mm%10); line[n++]=':'; line[n++]=(char)('0'+ss/10); line[n++]=(char)('0'+ss%10); line[n]=0;
      center_text(w,cy+58.0f,2.4f,line,1.0f,1.0f,1.0f,0.85f); }
    { float y=cy+110.0f; char buf[12]; char line[32]; int n;
      n=0; { const char*s="LEVEL X200 : "; while(*s)line[n++]=*s++; } fmt_int(buf,g_ocLevel*200); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
      center_text(w,y,2.0f,line,0.5f,1.0f,0.7f,0.85f);
      n=0; { const char*s="KILLS X5 : "; while(*s)line[n++]=*s++; } fmt_int(buf,g_kills*5); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
      center_text(w,y+26.0f,2.0f,line,0.6f,0.95f,1.0f,0.85f);
      n=0; { const char*s="BOSS X1000 : "; while(*s)line[n++]=*s++; } fmt_int(buf,g_bossKills*1000); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
      center_text(w,y+52.0f,2.0f,line,1.0f,0.5f,0.8f,0.85f);
      n=0; { const char*s="SCORE "; while(*s)line[n++]=*s++; } fmt_int(buf,oc_score()); { const char*p=buf; while(*p)line[n++]=*p++; } line[n]=0;
      center_text(w,y+96.0f,3.2f,line,1.0f,1.0f,1.0f,1.0f);
      n=0; { const char*s="( BEST "; while(*s)line[n++]=*s++; } fmt_int(buf,g_bestScore); { const char*p=buf; while(*p)line[n++]=*p++; } { const char*s2=" )"; while(*s2)line[n++]=*s2++; } line[n]=0;
      center_text(w,y+132.0f,1.8f,line,0.6f,0.9f,1.0f,0.75f);
    }
  } else {
  { char buf[12]; char line[28]; int n=0; const char*s="LAYER "; while(*s)line[n++]=*s++;
    fmt_int(buf,g_depth); { const char*p=buf; while(*p)line[n++]=*p++; }
    { const char*s2=" REACHED"; while(*s2)line[n++]=*s2++; } line[n]=0;
    center_text(w,cy+58.0f,2.4f,line,1.0f,1.0f,1.0f,0.85f); }
  if(g_deathMsg) center_text(w,cy+90.0f,1.6f,g_deathMsg,0.75f,0.5f,0.55f,0.8f); /* 서사: 사망 에피타프 */
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
  { char line[80]; int n=0; const char*s1="S SHAKE "; while(*s1)line[n++]=*s1++;
    { char b2[8]; fmt_int(b2,(int)(g_optShake*100.0f)); const char*p=b2; while(*p)line[n++]=*p++; }
    { const char*s2="  F FLASH "; while(*s2)line[n++]=*s2++; }
    { char b2[8]; fmt_int(b2,(int)(g_optFlash*100.0f)); const char*p=b2; while(*p)line[n++]=*p++; }
    { const char*s3="  C CRT "; while(*s3)line[n++]=*s3++; }
    { char b2[8]; fmt_int(b2,(int)(g_optCRT*100.0f)); const char*p=b2; while(*p)line[n++]=*p++; }
    { const char*s5="  M MUTE "; while(*s5)line[n++]=*s5++; }
    { const char*s4=g_mute?"ON":"OFF"; while(*s4)line[n++]=*s4++; }
    { const char*s6="  B BGM "; while(*s6)line[n++]=*s6++; }
    { const char*s7=g_bgm?"ON":"OFF"; while(*s7)line[n++]=*s7++; }
    line[n]=0;
    center_text(w,(float)h*0.63f,1.6f,line,0.6f,0.9f,1.0f,0.6f);
  }
  /* OVERCLOCK 빌드 요약 (docs/07 §7): 보유 무기/레벨·진화 */
  if(g_mode==MODE_OVERCLOCK){
    center_text(w,(float)h*0.72f,1.6f,"- ARSENAL -",0.5f,1.0f,0.7f,0.7f);
    int wi, shown=0;
    for(wi=0;wi<WEAPN;wi++){ if(!g_weapLvl[wi]) continue;
      char line[40]; int n=0; const char*nm=g_weapName[wi]; while(*nm)line[n++]=*nm++; line[n++]=' ';
      if(g_weapEvo[wi]){ const char*e="*EVO*"; while(*e)line[n++]=*e++; line[n]=0; }
      else { line[n++]='L'; line[n++]='V'; line[n++]=' '; char nb[8]; fmt_int(nb,g_weapLvl[wi]); const char*p=nb; while(*p)line[n++]=*p++; line[n]=0; }
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
      if(m>=100){ int wid=m-100; nm=g_weapName[wid]; ds=g_weapDesc[wid]; lvl=g_weapLvl[wid]; } /* OC 무기 카드 */
      else { nm=g_modName[m]; ds=g_modDesc[m]; lvl=g_mod[m]; }
      draw_text(x+(cw-text_w(nm,2.0f))*0.5f,y0+52.0f,2.0f,nm,1.0f,1.0f,1.0f,1.0f);
      draw_text(x+(cw-text_w(ds,1.4f))*0.5f,y0+92.0f,1.4f,ds,0.6f,0.95f,1.0f,0.85f);
      if(lvl>0){ char sb[10]; sb[0]='L'; sb[1]='V'; sb[2]=' '; fmt_int(sb+3,lvl+1);
        draw_text(x+(cw-text_w(sb,1.4f))*0.5f,y0+120.0f,1.4f,sb,1.0f,0.75f,0.3f,0.8f); }
    }
  }
  center_text(w,y0+ch+44.0f,1.8f,"CLICK OR 1 - 2 - 3",1.0f,1.0f,1.0f,0.5f);
}

