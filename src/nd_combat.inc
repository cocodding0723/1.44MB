/* 적 1체 AI·이동·접촉 (combat_update 적 루프 본문 추출 — 동작 동일, 적 추가 시 타입 분기만) */
static void enemy_update(int i, float dt){
  int j; Enemy*e=&g_ene[i];
    if(e->spawn>0.0f){ e->spawn-=dt; return; } /* 스폰 텔레그래프 중 */
    if(e->flash>0.0f) e->flash-=dt;
    float r=ene_radius(e->type), spd=ene_speed(e->type);
    if(e->affix&2) spd*=1.4f; /* 신속 (Swift) */
    float dx=g_player.pos.x-e->pos.x, dy=g_player.pos.y-e->pos.y, dist=f_sqrt(dx*dx+dy*dy); if(dist<0.01f)dist=0.01f;
    if(g_mod[M_MAGNET]){ float ar=80.0f+20.0f*(float)g_mod[M_MAGNET]; if(dist<ar) spd*=0.72f; } /* 자력 소형 감속 오라 (§7) */
    if(e->slowT>0.0f){ e->slowT-=dt; spd*=0.5f; } /* 냉기탄(M_CRYO) 둔화: 속도 50% */
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
    } else if(e->type==7){ /* PHANTOM: 주기 순간이동 교란 (v1.5) */
      e->t2+=dt;
      if(e->t2>=2.2f){ /* 블링크: 플레이어 방향 ~200px (오버슛/벽 진입 방지) */
        e->t2=0.0f;
        float bd=200.0f; if(bd>dist-40.0f) bd=dist-40.0f; if(bd<0.0f)bd=0.0f;
        float nx=e->pos.x+dx/dist*bd, ny=e->pos.y+dy/dist*bd;
        if(!(is_wall_w(nx-r*0.6f,ny)||is_wall_w(nx+r*0.6f,ny)||is_wall_w(nx,ny-r*0.6f)||is_wall_w(nx,ny+r*0.6f))){ e->pos.x=nx; e->pos.y=ny; } /* 4점 검사: 본체 벽 겹침 방지(리뷰 MED#1) */
        spawn_ring(e->pos.x,e->pos.y,16.0f,300.0f,0.3f,0.82f,0.78f,1.0f); burst(e->pos.x,e->pos.y,8,180.0f,0.85f,0.8f,1.0f);
      } else if(e->t2<1.7f){ /* 느린 드리프트 (벽 ±0.6r 2점 충돌) */
        float vx=dx/dist*spd*0.5f+kx, vy=dy/dist*spd*0.5f+ky;
        float nx=e->pos.x+vx*dt, ex2=nx+(vx>0?r:-r);
        if(!(is_wall_w(ex2,e->pos.y-r*0.6f)||is_wall_w(ex2,e->pos.y+r*0.6f))) e->pos.x=nx;
        float ny=e->pos.y+vy*dt, ey2=ny+(vy>0?r:-r);
        if(!(is_wall_w(e->pos.x-r*0.6f,ey2)||is_wall_w(e->pos.x+r*0.6f,ey2))) e->pos.y=ny;
      } /* 1.7~2.2s = 텔레그래프(정지, 렌더가 목적지 고스트 표시) */
    }
    /* 접촉 데미지 (+ 가시 모듈: 접촉 적 반격) */
    float cr=r+g_player.radius;
    if(dx*dx+dy*dy<cr*cr){
      if(g_mod[M_THORNS]&&g_thornT<=0.0f){
        g_thornT=0.25f;
        e->hp-=15.0f*(float)g_mod[M_THORNS]; e->flash=0.07f;
        e->kvel.x-=dx/dist*260.0f; e->kvel.y-=dy/dist*260.0f;
        burst(e->pos.x,e->pos.y,6,170.0f,0.4f,1.0f,0.6f);
        if(e->hp<=0.0f){ kill_enemy(i); return; }
      }
      hurt_player(1.0f);
    }
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
  /* 발사 (LMB 홀드 / OVERCLOCK 자동조준) — 모듈 수식(§6) + 총구 플래시 + 카메라 킥 */
  int ocFire=0; float ocAx=0.0f, ocAy=0.0f;
  if(g_mode==MODE_OVERCLOCK && !g_mouseDown){ int ti=acquire_target(g_player.pos.x,g_player.pos.y,900.0f);
    if(ti>=0){ ocFire=1; ocAx=g_ene[ti].pos.x; ocAy=g_ene[ti].pos.y; } }
  if((g_mouseDown||ocFire) && g_fireCd<=0.0f){
    float plv=(g_mode==MODE_OVERCLOCK&&g_weapLvl[W_PULSE]>0)?(float)(g_weapLvl[W_PULSE]-1):0.0f; /* PULSE 레벨 = 연사/탄수 보너스 */
    g_fireCd=FIRE_INT/(1.0f+0.20f*(float)g_mod[M_RAPID]+g_frenzy+0.18f*plv);
    float mwx,mwy;
    if(g_mouseDown){ mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX; mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY; }
    else { mwx=ocAx; mwy=ocAy; }
    float dx=mwx-g_player.pos.x, dy=mwy-g_player.pos.y, l=f_sqrt(dx*dx+dy*dy); if(l<0.001f)l=1.0f;
    dx/=l; dy/=l;
    float bx=g_player.pos.x+dx*16.0f, by=g_player.pos.y+dy*16.0f;
    int nsh=1+(g_mod[M_MULTI]>6?6:g_mod[M_MULTI]), s;
    if(g_mode==MODE_OVERCLOCK) nsh+=(int)((plv+1.0f)/2.0f); /* PULSE: lv2 +1탄, 2레벨당 +1 (QA: 정수나눗셈 0 버그 수정) */
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
        if(g_mode==MODE_OVERCLOCK&&g_weapEvo[W_PULSE]) b->pierce=(unsigned char)(b->pierce+2); /* PULSE 진화: 관통+2 */
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
        if(g_mod[M_CRYO]) e->slowT=0.5f+0.3f*(float)g_mod[M_CRYO]; /* 냉기탄(레어): 피격 적 둔화 */
        if(g_mod[M_EXECUTE]&&e->hp>0.0f&&e->hp<e->maxhp*(0.10f+0.04f*(float)g_mod[M_EXECUTE])) e->hp=0.0f; /* 처형(레어): 저HP 적 즉사 */
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
  for(i=0;i<MAXENE;i++){ if(!g_ene[i].active) continue; enemy_update(i,dt); }

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
    if(g_mode==MODE_OVERCLOCK&&p->kind==2) mag*=2.4f; /* OC XP 흡인 강화 (방클리어 수거 없음 — 페이싱) */
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
      } else if(p->kind==2){ /* OVERCLOCK XP 조각 (docs/07 §3.4) */
        g_ocXP+=2;
        spawn_part(p->pos.x,p->pos.y,0,-60.0f,0.2f,3.0f,0.4f,1.0f,0.6f);
        snd_play(SFX_COIN);
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

  /* OVERCLOCK 시스템 (자동무기·호드·보스웨이브·레벨업) */
  if(g_mode==MODE_OVERCLOCK){ oc_weapons_update(dt); oc_horde_update(dt); oc_boss_waves(dt); oc_level_check(); }

  /* 전투방 잠금/전멸/개방 (DESCENT 전용 — 던전 방·다운링크 로직) */
  if(g_mode==MODE_DESCENT){
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
      int kk=g_depth/3, m3=(kk-1)%3, btype=(m3==0)?0:(m3==1)?1:2; /* CORE(d3)/WARDEN(d6)/NEXUS(d9) 순환 */
      g_finalBoss=(g_depth>=12)?1:0; /* L12 KERNEL = 최종 CORE (§08 N3b) */
      if(g_finalBoss){ btype=0; set_xmit("THE CORE: YOU REACHED ME.\nFEW DO. NONE LEAVE.\nLET US END THIS, REVENANT."); }
      else set_xmit(g_xmitBoss[btype]); /* 서사: 보스 인트로 */
      boss_setup_bounds((float)(c->rx+1)*TILEF,(float)(c->rx+c->rw-1)*TILEF,(float)(c->ry+1)*TILEF,(float)(c->ry+c->rh-1)*TILEF);
      boss_spawn(btype,kk,g_finalBoss?1.4f:1.0f,0.5f*(float)(c->rw>c->rh?c->rw:c->rh)*TILEF);
      g_locked=1; g_lockGX=cgx; g_lockGY=cgy;
      snd_play(SFX_PHASE); snd_play(SFX_DOORCLOSE); add_trauma(0.3f);
    }
  }
  /* 상점 구매 (§13): 제단 근접 + E + BITS 충분 → 구매 */
  if(g_shopGX>=0 && g_wantBuy){ g_wantBuy=0; int si;
    for(si=0;si<3;si++){ if(g_shopBought[si]) continue;
      float spx,spy; shop_ped(si,&spx,&spy);
      float sdx=g_player.pos.x-spx, sdy=g_player.pos.y-spy;
      if(sdx*sdx+sdy*sdy<45.0f*45.0f && g_bits>=g_shopPrice[si]){
        g_bits-=g_shopPrice[si]; g_shopBought[si]=1;
        spawn_ring(spx,spy,12.0f,300.0f,0.3f,1.0f,0.7f,0.3f); snd_play(SFX_PICKUP);
        if(si==0){ draw3(0); g_upgRare=0; g_state=ST_UPG; }                 /* 모듈 3택1 */
        else if(si==1){ g_pHP+=2.0f; if(g_pHP>g_pMaxHP)g_pHP=g_pMaxHP; }    /* 수리 1하트 */
        else { int m=-1,tr; for(tr=0;tr<20;tr++){ int c2=(int)(xrnd()%MOD_COMMON); if(!mod_capped(c2)){ m=c2; break; } } if(m>=0) g_mod[m]++; } /* 리롤 랜덤 모듈 */
        break;
      }
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
  } /* end MODE_DESCENT room/descend gate */
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
