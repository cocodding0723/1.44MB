static void boss_setup_bounds(float mnx,float mxx,float mny,float mxy){
  g_bossMnx=mnx; g_bossMxx=mxx; g_bossMny=mny; g_bossMxy=mxy;
  g_bossCx=(mnx+mxx)*0.5f; g_bossCy=(mny+mxy)*0.5f;
}
static void boss_spawn(int type,int tier,float hpmul,float shrink0){
  g_boss.active=1; g_boss.type=(unsigned char)type;
  g_boss.maxhp=g_boss.hp=400.0f*(1.0f+0.5f*(float)(tier-1))*hpmul;
  g_boss.phase=1; g_boss.flash=0; g_boss.dashState=0;
  g_boss.pos.x=g_bossCx; g_boss.pos.y=g_bossCy;
  g_boss.t1=0; g_boss.t2=0; g_boss.t3=0; g_boss.segAng=0; g_boss.trailT=0; g_boss.stateT=0;
  g_boss.ldir.x=1; g_boss.ldir.y=0; g_boss.wdir.x=1; g_boss.wdir.y=0; g_boss.dashDir.x=1; g_boss.dashDir.y=0;
  g_boss.shrink=shrink0;
}
static void boss_die(void){
  int i;
  g_boss.active=0; g_bossDead=1; g_bossKills++; g_bgmIntense=0;
  g_hitstop=0.2f; add_trauma(0.8f);
  burst(g_boss.pos.x,g_boss.pos.y,48,420.0f,1.0f,0.9f,0.9f);
  spawn_ring(g_boss.pos.x,g_boss.pos.y,20.0f,700.0f,0.5f,1.0f,1.0f,1.0f);
  spawn_ring(g_boss.pos.x,g_boss.pos.y,10.0f,420.0f,0.4f,1.0f,0.5f,0.7f);
  snd_play(SFX_BOSSDIE);
  for(i=0;i<MAXENE;i++) if(g_ene[i].active){ g_ene[i].active=0; burst(g_ene[i].pos.x,g_ene[i].pos.y,8,180.0f,1.0f,0.6f,0.3f); }
  for(i=0;i<MAXEBUL;i++) g_ebul[i].active=0;
  for(i=0;i<MAXTRAIL;i++) g_trail[i].active=0;
  g_pHP+=2.0f; if(g_pHP>g_pMaxHP)g_pHP=g_pMaxHP; /* §9.3 보상 */
  unlock_codex(5); if(g_boss.type==1)unlock_codex(2); if(g_boss.type==2)unlock_codex(7); /* 코덱스: 보스 처치 */
  if(g_mode==MODE_OVERCLOCK){ /* OVERCLOCK: XP 폭발 + 즉시 1레벨 보장 (docs/07 §3.4) */
    int q; for(q=0;q<14;q++) spawn_pickup(g_boss.pos.x+rndsym()*40.0f,g_boss.pos.y+rndsym()*40.0f,2);
    g_ocBossT=120.0f; g_ocXP+=g_ocXPneed;
    return;
  }
  if(g_finalBoss){ /* 최종 CORE 격파 → 엔딩 (§08 N3b) */
    if(g_depth>g_bestLayer)g_bestLayer=g_depth; { int sc=score_now(); if(sc>g_bestScore)g_bestScore=sc; }
    unlock_codex(7); g_finalBoss=0;
    g_ending=compute_ending(); g_endMsg=g_endText[g_ending];
    save_write(); g_state=ST_ENDING; return;
  }
  draw3(1); g_upgRare=1; g_state=ST_UPG;
}
static void boss_hit(float dmg){
  if(!g_boss.active) return;
  g_boss.hp-=dmg; g_boss.flash=0.06f;
  if(g_boss.hp<=0.0f) boss_die();
}

static void player_update(float dt){
  vec2 in={0.0f,0.0f};
  if(g_keys['W']||g_keys[VK_UP])    in.y-=1.0f;
  if(g_keys['S']||g_keys[VK_DOWN])  in.y+=1.0f;
  if(g_keys['A']||g_keys[VK_LEFT])  in.x-=1.0f;
  if(g_keys['D']||g_keys[VK_RIGHT]) in.x+=1.0f;
  float il=f_sqrt(in.x*in.x+in.y*in.y);
  if(il>0.0001f){ in.x/=il; in.y/=il; }

  /* 대시 (§5: 700px/s 0.14s, i-frame, 쿨 0.8s×냉각, 잔상) */
  if(g_dashCd>0.0f) g_dashCd-=dt;
  if(g_wantDash && g_dashCd<=0.0f && g_dashT<=0.0f){
    g_wantDash=0;
    vec2 dd=in;
    if(il<=0.0001f){ /* 입력 없으면 마우스 방향 */
      float mwx=g_cam.x-g_winW*0.5f+(float)g_mouseX, mwy=g_cam.y-g_winH*0.5f+(float)g_mouseY;
      dd.x=mwx-g_player.pos.x; dd.y=mwy-g_player.pos.y;
      float dl=f_sqrt(dd.x*dd.x+dd.y*dd.y); if(dl<0.001f){dd.x=1.0f;dd.y=0;} else {dd.x/=dl; dd.y/=dl;}
    }
    g_dashT=DASH_TIME; g_dashCd=dash_cd_max(); g_dashDir=dd; g_afterTimer=0;
    spawn_ring(g_player.pos.x,g_player.pos.y,6.0f,300.0f,0.25f,0.4f,0.95f,1.0f);
    snd_play(SFX_DASH);
  }

  if(g_dashT>0.0f){ /* 대시 중: 고정 속도 + 잔상 */
    g_dashT-=dt;
    g_player.vel.x=g_dashDir.x*DASH_SPEED; g_player.vel.y=g_dashDir.y*DASH_SPEED;
    g_afterTimer-=dt;
    if(g_afterTimer<=0.0f){ g_afterTimer=0.03f; spawn_after(g_player.pos.x,g_player.pos.y); }
  } else {
    if(il>0.0001f){ g_player.vel.x+=in.x*ACCEL*dt; g_player.vel.y+=in.y*ACCEL*dt; }
    else { float s=f_sqrt(g_player.vel.x*g_player.vel.x+g_player.vel.y*g_player.vel.y);
      if(s>0.0001f){ float ns=s-FRICTION*dt; if(ns<0.0f)ns=0.0f; g_player.vel.x=g_player.vel.x/s*ns; g_player.vel.y=g_player.vel.y/s*ns; } }
    float msp=MAXSPEED*(1.0f+0.12f*(float)g_mod[M_AGI]); if(msp>MAXSPEED*1.8f)msp=MAXSPEED*1.8f; /* 민첩 (cap 1.8x) */
    float sp=f_sqrt(g_player.vel.x*g_player.vel.x+g_player.vel.y*g_player.vel.y);
    if(sp>msp){ g_player.vel.x=g_player.vel.x/sp*msp; g_player.vel.y=g_player.vel.y/sp*msp; }
  }
  float r=g_player.radius;
  float nxp=g_player.pos.x+g_player.vel.x*dt, ex=nxp+(g_player.vel.x>0.0f?r:-r);
  if(is_wall_w(ex,g_player.pos.y-r*0.6f)||is_wall_w(ex,g_player.pos.y+r*0.6f)){
    if(g_dashT>0.0f){ /* 대시 충돌로 크레이트 파괴 (docs/06 §3.3) */
      if(try_break_crate(ex,g_player.pos.y-r*0.6f)|try_break_crate(ex,g_player.pos.y+r*0.6f)) add_trauma(0.1f); }
    if(is_wall_w(ex,g_player.pos.y-r*0.6f)||is_wall_w(ex,g_player.pos.y+r*0.6f)) g_player.vel.x=0.0f; else g_player.pos.x=nxp;
  } else g_player.pos.x=nxp;
  float nyp=g_player.pos.y+g_player.vel.y*dt, ey=nyp+(g_player.vel.y>0.0f?r:-r);
  if(is_wall_w(g_player.pos.x-r*0.6f,ey)||is_wall_w(g_player.pos.x+r*0.6f,ey)){
    if(g_dashT>0.0f){
      if(try_break_crate(g_player.pos.x-r*0.6f,ey)|try_break_crate(g_player.pos.x+r*0.6f,ey)) add_trauma(0.1f); }
    if(is_wall_w(g_player.pos.x-r*0.6f,ey)||is_wall_w(g_player.pos.x+r*0.6f,ey)) g_player.vel.y=0.0f; else g_player.pos.y=nyp;
  } else g_player.pos.y=nyp;
}

static void kill_enemy(int idx){
  Enemy*e=&g_ene[idx];
  e->active=0; g_kills++;
  g_hitstop=0.03f; add_trauma(0.12f);
  snd_play(SFX_DEATH);
  burst(e->pos.x,e->pos.y,16,240.0f,1.0f,0.5f,0.2f);
  spawn_ring(e->pos.x,e->pos.y,6.0f,360.0f,0.3f,1.0f,0.6f,0.25f);
  if(e->type==3){ /* 포크: Shard 2 분열 (재분열 없음, §8.4; 벽 내부 스폰 폴백) */
    float lx=is_wall_w(e->pos.x-10.0f,e->pos.y)?e->pos.x:e->pos.x-10.0f;
    float rx=is_wall_w(e->pos.x+10.0f,e->pos.y)?e->pos.x:e->pos.x+10.0f;
    int s1=spawn_enemy(4,lx,e->pos.y,0), s2=spawn_enemy(4,rx,e->pos.y,0);
    if(s1>=0) g_ene[s1].spawn=0.15f;
    if(s2>=0) g_ene[s2].spawn=0.15f;
  }
  if(e->affix&4){ /* 폭발사망 (Volatile): 0.5s 지연 폭탄 */
    int q; for(q=0;q<MAXBOOM;q++) if(!g_boom[q].active){ g_boom[q].active=1; g_boom[q].pos=e->pos; g_boom[q].t=g_boom[q].t0=0.5f; break; }
  }
  if(g_mode==MODE_OVERCLOCK){ /* OVERCLOCK: XP 조각 드롭 (BITS/흡혈/흡수 경제 미사용, docs/07 §3.4) */
    int xc=(e->type==3)?3:((e->type==5||e->type==6)?2:1); if(e->affix)xc+=2;
    int q; for(q=0;q<xc;q++) spawn_pickup(e->pos.x,e->pos.y,2);
    return;
  }
  int nb=1+(int)(xrnd()%3), b;
  if(e->affix){ nb*=3; /* 엘리트 BITS ×3 + 깊은 레이어 8% 모듈 오브 */
    if(g_depth>=5&&(xrnd()%100)<8) spawn_pickup(e->pos.x,e->pos.y,1);
  }
  for(b=0;b<nb;b++) spawn_pickup(e->pos.x,e->pos.y,0);
  if(g_mod[M_SIPHON]){ /* 흡수: N킬마다 보너스 BITS 다발 + 광란 너지 (회복 아님, §7) */
    int th=5-(g_mod[M_SIPHON]-1); if(th<2)th=2;
    g_siphonK++;
    if(g_siphonK>=th){ g_siphonK=0; int extra=2+g_mod[M_SIPHON]; if(extra>5)extra=5; int q;
      for(q=0;q<extra;q++) spawn_pickup(e->pos.x,e->pos.y,0);
      if(g_mod[M_FRENZY]){ g_frenzy+=0.10f; float fm=0.5f*(float)g_mod[M_FRENZY]; if(fm>1.5f)fm=1.5f; if(g_frenzy>fm)g_frenzy=fm; } } /* 광란 너지는 FRENZY 보유 시만 (캡 존중) */
  }
  if(g_mod[M_LEECH]){ /* 흡혈: 12킬마다 1HP (스택당 -3, 최소 3) */
    int th=12-3*(g_mod[M_LEECH]-1); if(th<3)th=3;
    g_leechKills++;
    if(g_leechKills>=th){ g_leechKills=0;
      if(g_pHP<g_pMaxHP){ g_pHP+=1.0f; spawn_ring(g_player.pos.x,g_player.pos.y,10.0f,180.0f,0.3f,0.3f,1.0f,0.5f); } }
  }
}
static void explode_at(float x,float y,float bdmg){ /* 폭발 모듈 (§7, 자해 없음) */
  int stk=g_mod[M_EXPLODE], j; if(stk<=0) return;
  float R=60.0f+25.0f*(float)(stk-1), dm=bdmg*(0.50f+0.15f*(float)(stk-1));
  for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
    float dx=e->pos.x-x, dy=e->pos.y-y, rr=R+ene_radius(e->type);
    if(dx*dx+dy*dy<rr*rr){ float d=f_sqrt(dx*dx+dy*dy); if(d<0.01f)d=0.01f;
      e->hp-=dm; e->flash=0.07f; e->kvel.x+=dx/d*120.0f; e->kvel.y+=dy/d*120.0f;
      if(e->hp<=0.0f) kill_enemy(j); } }
  if(g_boss.active){ float bdx=g_boss.pos.x-x, bdy=g_boss.pos.y-y, rr=R+boss_radius();
    if(bdx*bdx+bdy*bdy<rr*rr) boss_hit(dm); }
  burst(x,y,12,260.0f,1.0f,0.6f,0.2f);
  spawn_ring(x,y,10.0f,(R-10.0f)*5.5f,0.18f,1.0f,0.55f,0.2f);
  add_trauma(0.25f);
}
static void arc_chain(float x,float y,int skip,float dmg){ /* 체인라이트닝 (레어): 인접 1체에 단발 아크(재귀 없음, §7) */
  int j,bj=-1; float bd2=140.0f*140.0f;
  for(j=0;j<MAXENE;j++){ if(j==skip||!g_ene[j].active||g_ene[j].spawn>0.0f) continue;
    float dx=g_ene[j].pos.x-x, dy=g_ene[j].pos.y-y, d2=dx*dx+dy*dy;
    if(d2<bd2){ bd2=d2; bj=j; } }
  if(bj<0) return;
  Enemy*e=&g_ene[bj]; e->hp-=dmg; e->flash=0.06f;
  spawn_ring(e->pos.x,e->pos.y,ene_radius(e->type)+4.0f,200.0f,0.18f,0.5f,0.9f,1.0f);
  burst(e->pos.x,e->pos.y,4,160.0f,0.5f,0.9f,1.0f);
  if(e->hp<=0.0f) kill_enemy(bj);
}

static void boss_update(float dt){
  if(!g_boss.active) return;
  Boss*B=&g_boss; int k;
  float br=boss_radius();
  float mnx=g_bossMnx+br, mxx=g_bossMxx-br;
  float mny=g_bossMny+br, mxy=g_bossMxy-br;
  float rcx=g_bossCx, rcy=g_bossCy;
  if(B->flash>0.0f) B->flash-=dt;
  float hpr=B->hp/B->maxhp;
  int ph=hpr>0.66f?1:(hpr>0.33f?2:3);
  g_bgmIntense=(ph==3); /* BGM P3 격화 (§16.4) */
  if(ph!=B->phase){ B->phase=ph; snd_play(SFX_PHASE); add_trauma(0.3f); B->flash=0.12f; g_fringeFx=0.5f;
    spawn_ring(B->pos.x,B->pos.y,br,500.0f,0.4f,1.0f,0.4f,0.4f);
    spawn_ring(B->pos.x,B->pos.y,br*0.4f,720.0f,0.5f,1.0f,1.0f,1.0f);
    burst(B->pos.x,B->pos.y,18,300.0f,1.0f,0.5f,0.3f); }
  float dx=g_player.pos.x-B->pos.x, dy=g_player.pos.y-B->pos.y;
  float dist=f_sqrt(dx*dx+dy*dy); if(dist<0.01f)dist=0.01f;

  if(B->type==0){ /* === THE CORE: 탄막형 (§9.1) === */
    float mv=ph==3?60.0f:30.0f;
    B->pos.x+=dx/dist*mv*dt; B->pos.y+=dy/dist*mv*dt;
    if(B->pos.x<mnx)B->pos.x=mnx; if(B->pos.x>mxx)B->pos.x=mxx;
    if(B->pos.y<mny)B->pos.y=mny; if(B->pos.y>mxy)B->pos.y=mxy;
    B->t1+=dt;
    float ringInt=ph==3?1.5f:2.5f;
    if(B->t1>=ringInt){ B->t1=0.0f; B->segAng+=0.4f; /* 16발 방사 링 (회전 오프셋) */
      for(k=0;k<16;k++){ float a=B->segAng+(float)k*0.3926991f;
        spawn_ebul(B->pos.x,B->pos.y,f_cos(a),f_sin(a),220.0f,4.0f); }
      snd_play(SFX_SHOOT);
    }
    if(ph>=2){
      if(B->t2<dt){ B->ldir.x=dx/dist; B->ldir.y=dy/dist; } /* 사이클 시작: 조준 */
      B->t2+=dt;
      if(B->t2>=0.8f&&B->t2<1.2f){ /* 발사: 150°/s 스윕 */
        vrot(&B->ldir,2.618f*dt);
        if(beam_hit(B->pos.x,B->pos.y,B->ldir.x,B->ldir.y,7.0f,g_player.pos.x,g_player.pos.y,g_player.radius,700.0f)) hurt_player(2.0f);
        if(ph==3&&beam_hit(B->pos.x,B->pos.y,-B->ldir.x,-B->ldir.y,7.0f,g_player.pos.x,g_player.pos.y,g_player.radius,700.0f)) hurt_player(2.0f);
      }
      if(B->t2>=5.0f) B->t2=0.0f;
      B->t3+=dt;
      if(B->t3>=6.0f){ B->t3=0.0f; /* 헌터 2 소환 */
        spawn_enemy(0,B->pos.x-60.0f,B->pos.y,0); spawn_enemy(0,B->pos.x+60.0f,B->pos.y,0); }
    }
  } else if(B->type==1){ /* === THE WARDEN: 돌진/공간압축형 (§9.2) === */
    int nseg=ph==3?3:2;
    float segRate=(ph==1?90.0f:(ph==2?130.0f:70.0f))*0.0174533f;
    B->segAng+=segRate*dt;
    for(k=0;k<nseg;k++){ /* 공전 세그먼트 접촉뎀 1 */
      float a=B->segAng+(float)k*6.2831853f/(float)nseg;
      float sx=B->pos.x+f_cos(a)*130.0f, sy=B->pos.y+f_sin(a)*130.0f;
      float sdx=sx-g_player.pos.x, sdy=sy-g_player.pos.y, rr=16.0f+g_player.radius;
      if(sdx*sdx+sdy*sdy<rr*rr) hurt_player(1.0f);
    }
    float idleT=ph==1?1.6f:(ph==2?1.2f:0.9f);
    if(B->dashState==0){ /* 부유 + 돌진 준비 */
      B->pos.x+=dx/dist*40.0f*dt; B->pos.y+=dy/dist*40.0f*dt;
      B->stateT+=dt;
      if(B->stateT>=idleT){ B->stateT=0.0f; B->dashState=1; B->dashDir.x=dx/dist; B->dashDir.y=dy/dist; }
    } else if(B->dashState==1){ /* 0.7s 조준 텔레그래프 */
      B->stateT+=dt;
      if(B->stateT>=0.7f){ B->stateT=0.0f; B->dashState=2; snd_play(SFX_DASH); }
    } else if(B->dashState==2){ /* 돌진 900px/s + 잔류 트레일 */
      B->pos.x+=B->dashDir.x*900.0f*dt; B->pos.y+=B->dashDir.y*900.0f*dt;
      B->trailT-=dt;
      if(B->trailT<=0.0f){ B->trailT=0.04f;
        for(k=0;k<MAXTRAIL;k++) if(!g_trail[k].active){ g_trail[k].active=1; g_trail[k].pos=B->pos; g_trail[k].life=0.6f; break; } }
      if(B->pos.x<mnx||B->pos.x>mxx||B->pos.y<mny||B->pos.y>mxy){
        if(B->pos.x<mnx)B->pos.x=mnx; if(B->pos.x>mxx)B->pos.x=mxx;
        if(B->pos.y<mny)B->pos.y=mny; if(B->pos.y>mxy)B->pos.y=mxy;
        B->dashState=3; B->stateT=0.0f; add_trauma(0.4f); snd_play(SFX_HIT);
        burst(B->pos.x,B->pos.y,14,260.0f,1.0f,0.6f,0.2f);
      }
    } else { B->stateT+=dt; if(B->stateT>=0.5f){ B->stateT=0.0f; B->dashState=0; } }
    if(ph>=2){ /* 회전 레이저 암 */
      int narm=ph==3?3:2;
      float armRate=(ph==2?45.0f:70.0f)*0.0174533f;
      vrot(&B->wdir,armRate*dt);
      for(k=0;k<narm;k++){ vec2 ad=B->wdir; vrot(&ad,(float)k*6.2831853f/(float)narm);
        if(beam_hit(B->pos.x,B->pos.y,ad.x,ad.y,7.0f,g_player.pos.x,g_player.pos.y,g_player.radius,700.0f)) hurt_player(2.0f); }
    }
    if(ph==3){ /* 압축 격노: 안전 반경 수축 + 상시 약한 셰이크 */
      if(B->shrink>180.0f) B->shrink-=8.0f*dt;
      if(g_trauma<0.08f) g_trauma=0.08f;
      float pdx=g_player.pos.x-rcx, pdy=g_player.pos.y-rcy;
      if(pdx*pdx+pdy*pdy>B->shrink*B->shrink) hurt_player(1.0f);
    }
  } else if(B->type==2){ /* === THE NEXUS: 노드망 빔케이지 존-컨트롤 (§9.3) === */
    int kk2; float nx3[3], ny3[3];
    if(B->shrink>150.0f) B->shrink=150.0f; /* 스폰 init 정규화 → 노드 반경 */
    float nd=B->shrink;
    B->segAng+=(ph==3?1.05f:(ph==2?0.79f:0.61f))*dt;
    B->pos.x+=dx/dist*25.0f*dt; B->pos.y+=dy/dist*25.0f*dt;
    if(B->pos.x<mnx)B->pos.x=mnx; if(B->pos.x>mxx)B->pos.x=mxx;
    if(B->pos.y<mny)B->pos.y=mny; if(B->pos.y>mxy)B->pos.y=mxy;
    for(kk2=0;kk2<3;kk2++){ float a=B->segAng+(float)kk2*2.0943951f; nx3[kk2]=B->pos.x+f_cos(a)*nd; ny3[kk2]=B->pos.y+f_sin(a)*nd; }
    float halfw=7.0f; /* 전 페이즈 동일 — ph3 압박은 케이지 수축으로 (보이지 않는 폭증 제거) */
    for(kk2=0;kk2<3;kk2++){ int k3=(kk2+1)%3; /* 회전 삼각 빔케이지 (3변) */
      float ex=nx3[k3]-nx3[kk2], ey=ny3[k3]-ny3[kk2], el=f_sqrt(ex*ex+ey*ey); if(el<0.01f)el=0.01f;
      if(beam_hit(nx3[kk2],ny3[kk2],ex/el,ey/el,halfw,g_player.pos.x,g_player.pos.y,g_player.radius,el)) hurt_player(2.0f); }
    if(ph>=2){ B->t1+=dt; /* 노드 방사 링 (밀도 완화: 노드당 8발) */
      if(B->t1>=(ph==3?3.6f:2.2f)){ B->t1=0.0f; int b2;
        for(kk2=0;kk2<3;kk2++) for(b2=0;b2<8;b2++){ float a=(float)b2*0.7853982f; spawn_ebul(nx3[kk2],ny3[kk2],f_cos(a),f_sin(a),240.0f,4.0f); }
        snd_play(SFX_PHASE); } } /* 36발→24발, 위협 사운드 */
    if(ph==3){ if(B->shrink>70.0f) B->shrink-=12.0f*dt; if(g_trauma<0.06f) g_trauma=0.06f; /* 수축 18→12px/s, 본체 링 16→12발 */
      B->t2+=dt; if(B->t2>=1.6f){ B->t2=0.0f; int b2; for(b2=0;b2<12;b2++){ float a=B->segAng+(float)b2*0.5235988f; spawn_ebul(B->pos.x,B->pos.y,f_cos(a),f_sin(a),220.0f,4.0f); } } }
    B->t3+=dt; if(B->t3>=8.0f){ B->t3=0.0f; int tc=0,tt; for(tt=0;tt<MAXENE;tt++) if(g_ene[tt].active&&g_ene[tt].type==1)tc++; /* 터릿 소환: 최대 2, 방 경계 클램프 */
      if(tc<2){ float tsx=nx3[0], tsy=ny3[0]; if(tsx<mnx)tsx=mnx; if(tsx>mxx)tsx=mxx; if(tsy<mny)tsy=mny; if(tsy>mxy)tsy=mxy; spawn_enemy(1,tsx,tsy,0); } }
    for(kk2=0;kk2<3;kk2++){ float ndx=nx3[kk2]-g_player.pos.x, ndy=ny3[kk2]-g_player.pos.y, rr=14.0f+g_player.radius; if(ndx*ndx+ndy*ndy<rr*rr) hurt_player(1.0f); }
  }
  /* 트레일 수명/접촉 */
  for(k=0;k<MAXTRAIL;k++){ if(!g_trail[k].active) continue;
    g_trail[k].life-=dt; if(g_trail[k].life<=0.0f){ g_trail[k].active=0; continue; }
    float tdx=g_trail[k].pos.x-g_player.pos.x, tdy=g_trail[k].pos.y-g_player.pos.y, rr=14.0f+g_player.radius;
    if(tdx*tdx+tdy*tdy<rr*rr) hurt_player(1.0f);
  }
  /* 보스 본체 접촉뎀 2 */
  if(g_boss.active&&dist<br+g_player.radius) hurt_player(2.0f);
}
static void consume_latches(void){ g_wantDash=0; g_wantEmp=0; g_wantBuy=0; } /* sim 스텝 끝에서만 소거 */
static void spawn_minib(float x,float y,float ux,float uy,float dmg){
  int i; for(i=0;i<MAXBUL;i++) if(!g_bul[i].active){ Bullet*b=&g_bul[i];
    b->active=1; b->pos.x=x; b->pos.y=y; b->vel.x=ux*500.0f; b->vel.y=uy*500.0f;
    b->life=0.5f; b->dmg=dmg; b->r=3.0f; b->pierce=0; b->bounce=0; b->mini=1; b->lastHit=0; return; }
}

/* ===================== OVERCLOCK MODE 시스템 (docs/07 O1 MVP) ===================== */
/* 최근접 타게팅 (§14.2): strict < → 동거리 시 최저 인덱스 우선, 제곱거리 비교(f_sqrt 0회) */
static int acquire_target(float ox,float oy,float maxR){
  int best=-1,j; float bd2=maxR*maxR;
  for(j=0;j<MAXENE;j++){ Enemy*e=&g_ene[j];
    if(!e->active||e->spawn>0.0f) continue;
    float dx=e->pos.x-ox, dy=e->pos.y-oy, d2=dx*dx+dy*dy;
    if(d2<bd2){ bd2=d2; best=j; } }
  return best;
}
/* 시간 게이팅 적 추첨 (§3.5): fodder 위주 → 점차 해금 */
static int oc_pick_type(void){
  int types[8], w[8], n=0, i, tot=0;
  types[n]=0; w[n++]=10;            /* HUNTER */
  types[n]=4; w[n++]=8;             /* SHARD = MITE fodder (§6) */
  if(g_ocT>=60.0f){ types[n]=1; w[n++]=5; types[n]=2; w[n++]=5; types[n]=5; w[n++]=4; }
  if(g_ocT>=180.0f){ types[n]=3; w[n++]=4; types[n]=6; w[n++]=4; types[n]=7; w[n++]=4; } /* +PHANTOM (v1.5) */
  for(i=0;i<n;i++) tot+=w[i];
  int r=(int)(xrnd()%(unsigned)tot);
  for(i=0;i<n;i++){ r-=w[i]; if(r<0) return types[i]; }
  return 0;
}
static int oc_spawn_at_edge(int type){
  int tr; for(tr=0;tr<30;tr++){
    int side=(int)(xrnd()&3); float ex,ey, m=24.0f;
    if(side==0){ ex=g_arenaX0+m+rnd01()*(g_arenaX1-g_arenaX0-2.0f*m); ey=g_arenaY0+m; }
    else if(side==1){ ex=g_arenaX1-m; ey=g_arenaY0+m+rnd01()*(g_arenaY1-g_arenaY0-2.0f*m); }
    else if(side==2){ ex=g_arenaX0+m+rnd01()*(g_arenaX1-g_arenaX0-2.0f*m); ey=g_arenaY1-m; }
    else { ex=g_arenaX0+m; ey=g_arenaY0+m+rnd01()*(g_arenaY1-g_arenaY0-2.0f*m); }
    float dx=ex-g_player.pos.x, dy=ey-g_player.pos.y;
    if(dx*dx+dy*dy<150.0f*150.0f) continue;
    if(is_wall_w(ex,ey)) continue;
    return spawn_enemy(type,ex,ey,1);
  }
  return -1;
}
/* 호드 스포너 (§3.5): 시간 t로 밀도·HP·종류 스케일. g_depth를 시간 파생으로 두어 기존 스케일 재활용 */
static void oc_horde_update(float dt){
  int j;
  g_ocT+=dt;
  g_depth=1+(int)(g_ocT/25.0f); if(g_depth>12)g_depth=12; /* HP/속도/엘리트 스케일 구동 */
  g_ocSpawnT-=dt;
  if(g_ocSpawnT<=0.0f){
    float interval=1.2f-g_ocT*0.004f; if(interval<0.15f)interval=0.15f;
    g_ocSpawnT=interval;
    int alive=0; for(j=0;j<MAXENE;j++) if(g_ene[j].active) alive++;
    int cap=40+(int)(g_ocT*0.5f); if(cap>MAXENE-8)cap=MAXENE-8;
    int batch=1+(int)(xrnd()%3), b;
    for(b=0;b<batch;b++){ if(alive>=cap)break; oc_spawn_at_edge(oc_pick_type()); alive++; }
  }
}
/* 자동 무기: ORBITERS(공전 블레이드) + NOVA(최근접 폭발). PULSE는 발사 블록에서 자동조준 */
static void oc_weapons_update(float dt){
  int j,k;
  g_orbAng+=dt*2.4f;
  if(g_weapLvl[W_ORBIT]){
    int lv=g_weapLvl[W_ORBIT], evo=g_weapEvo[W_ORBIT], n=2+lv+(evo?2:0); float R=70.0f+10.0f*(float)lv+(evo?25.0f:0.0f), odm=(18.0f+8.0f*(float)lv)*(evo?1.6f:1.0f);
    g_weapTimer[W_ORBIT]-=dt; int tick=g_weapTimer[W_ORBIT]<=0.0f; if(tick) g_weapTimer[W_ORBIT]=0.12f;
    if(tick) for(k=0;k<n;k++){ float a=g_orbAng+(float)k*6.2831853f/(float)n;
      float ox=g_player.pos.x+f_cos(a)*R, oy=g_player.pos.y+f_sin(a)*R;
      for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
        float dx=e->pos.x-ox, dy=e->pos.y-oy, rr=ene_radius(e->type)+10.0f;
        if(dx*dx+dy*dy<rr*rr){ float d=f_sqrt(dx*dx+dy*dy); if(d<0.01f)d=0.01f;
          e->hp-=odm; e->flash=0.06f; e->kvel.x+=dx/d*120.0f; e->kvel.y+=dy/d*120.0f;
          if(e->hp<=0.0f) kill_enemy(j); } }
      if(g_boss.active){ float bx=g_boss.pos.x-ox, by=g_boss.pos.y-oy, rr=boss_radius()+10.0f; if(bx*bx+by*by<rr*rr) boss_hit(odm); } }
  }
  if(g_weapLvl[W_NOVA]){
    int lv=g_weapLvl[W_NOVA], evo=g_weapEvo[W_NOVA];
    g_weapTimer[W_NOVA]-=dt;
    if(g_weapTimer[W_NOVA]<=0.0f){
      g_weapTimer[W_NOVA]=(2.5f-0.15f*(float)lv)*(evo?0.6f:1.0f); if(g_weapTimer[W_NOVA]<0.6f)g_weapTimer[W_NOVA]=0.6f;
      int ti=acquire_target(g_player.pos.x,g_player.pos.y,700.0f);
      if(ti>=0){ float nx=g_ene[ti].pos.x, ny=g_ene[ti].pos.y, R=(70.0f+12.0f*(float)lv)*(evo?1.4f:1.0f), dm=(40.0f+15.0f*(float)lv)*(evo?1.3f:1.0f);
        for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
          float dx=e->pos.x-nx, dy=e->pos.y-ny, rr=R+ene_radius(e->type);
          if(dx*dx+dy*dy<rr*rr){ e->hp-=dm; e->flash=0.07f; if(e->hp<=0.0f) kill_enemy(j); } }
        if(g_boss.active){ float bx=g_boss.pos.x-nx, by=g_boss.pos.y-ny, rr=R+boss_radius(); if(bx*bx+by*by<rr*rr) boss_hit(dm); }
        burst(nx,ny,16,300.0f,1.0f,0.6f,0.2f); spawn_ring(nx,ny,10.0f,R*5.0f,0.25f,1.0f,0.55f,0.2f);
        add_trauma(0.15f); snd_play(SFX_DEATH); }
    }
  }
  if(g_weapLvl[W_AURA]){ /* AURA: 반경 R 지속 장판, 0.25s 틱뎀+소형 넉백 */
    int lv=g_weapLvl[W_AURA], evo=g_weapEvo[W_AURA]; float R=(80.0f+18.0f*(float)lv)*(evo?1.3f:1.0f), dm=(8.0f+4.0f*(float)lv)*(evo?1.5f:1.0f);
    g_weapTimer[W_AURA]-=dt;
    if(g_weapTimer[W_AURA]<=0.0f){ g_weapTimer[W_AURA]=0.25f;
      for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
        float dx=e->pos.x-g_player.pos.x, dy=e->pos.y-g_player.pos.y, rr=R+ene_radius(e->type);
        if(dx*dx+dy*dy<rr*rr){ float d=f_sqrt(dx*dx+dy*dy); if(d<0.01f)d=0.01f;
          e->hp-=dm; e->flash=0.05f; e->kvel.x+=dx/d*60.0f; e->kvel.y+=dy/d*60.0f;
          if(e->hp<=0.0f) kill_enemy(j); } }
      if(g_boss.active){ float bx=g_boss.pos.x-g_player.pos.x, by=g_boss.pos.y-g_player.pos.y, rr=R+boss_radius(); if(bx*bx+by*by<rr*rr) boss_hit(dm); }
    }
  }
  if(g_weapLvl[W_SWARM]){ /* SWARM: 최근접 N체에 조준 나이프 일제 발사 (spawn_minib 재활용) */
    int lv=g_weapLvl[W_SWARM];
    g_weapTimer[W_SWARM]-=dt;
    if(g_weapTimer[W_SWARM]<=0.0f){ int evo=g_weapEvo[W_SWARM]; g_weapTimer[W_SWARM]=(1.4f-0.08f*(float)lv)*(evo?0.7f:1.0f); if(g_weapTimer[W_SWARM]<0.4f)g_weapTimer[W_SWARM]=0.4f;
      int nk=2+lv+(evo?2:0), used[18], uc=0, q, fired=0;
      for(q=0;q<nk;q++){
        int bestj=-1; float bd2=600.0f*600.0f;
        for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue;
          int skip=0,u; for(u=0;u<uc;u++) if(used[u]==j){ skip=1; break; } if(skip) continue;
          float dx=g_ene[j].pos.x-g_player.pos.x, dy=g_ene[j].pos.y-g_player.pos.y, d2=dx*dx+dy*dy;
          if(d2<bd2){ bd2=d2; bestj=j; } }
        if(bestj<0) break;
        if(uc<18) used[uc++]=bestj;
        float dx=g_ene[bestj].pos.x-g_player.pos.x, dy=g_ene[bestj].pos.y-g_player.pos.y, l=f_sqrt(dx*dx+dy*dy); if(l<0.01f)l=1.0f;
        spawn_minib(g_player.pos.x,g_player.pos.y,dx/l,dy/l,BUL_DMG*(0.6f+0.2f*(float)lv)); fired++;
      }
      if(fired) snd_play(SFX_SHOOT);
    }
  }
  if(g_weapLvl[W_BEAM]){ /* BEAM: 회전 스윕 레이저 (draw_beam/beam_hit 재활용) */
    int lv=g_weapLvl[W_BEAM], evo=g_weapEvo[W_BEAM], narm=1+lv/3+(evo?1:0), k; if(narm>4)narm=4;
    float len=240.0f+30.0f*(float)lv+(evo?60.0f:0.0f), dm=(14.0f+5.0f*(float)lv)*(evo?1.4f:1.0f);
    g_beamAng += (0.9f+0.05f*(float)lv)*dt;
    g_weapTimer[W_BEAM]-=dt; int tick=g_weapTimer[W_BEAM]<=0.0f; if(tick) g_weapTimer[W_BEAM]=0.1f;
    if(tick) for(k=0;k<narm;k++){ float a=g_beamAng+(float)k*6.2831853f/(float)narm, ux=f_cos(a), uy=f_sin(a);
      for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; Enemy*e=&g_ene[j];
        if(beam_hit(g_player.pos.x,g_player.pos.y,ux,uy,9.0f,e->pos.x,e->pos.y,ene_radius(e->type),len)){ e->hp-=dm; e->flash=0.05f; if(e->hp<=0.0f) kill_enemy(j); } }
      if(g_boss.active && beam_hit(g_player.pos.x,g_player.pos.y,ux,uy,9.0f,g_boss.pos.x,g_boss.pos.y,boss_radius(),len)) boss_hit(dm); }
  }
  if(g_weapLvl[W_ARC]){ /* ARC: 주기적 체인 라이트닝 (arc_chain 패턴, 방문셋 추적) */
    int lv=g_weapLvl[W_ARC];
    g_weapTimer[W_ARC]-=dt;
    if(g_weapTimer[W_ARC]<=0.0f){
      g_weapTimer[W_ARC]=1.2f-0.07f*(float)lv; if(g_weapTimer[W_ARC]<0.4f)g_weapTimer[W_ARC]=0.4f;
      int evo=g_weapEvo[W_ARC], hops=2+lv+(evo?2:0), vis[18], nv=0, hh; float cx=g_player.pos.x, cy=g_player.pos.y, dm=BUL_DMG*(0.5f+0.15f*(float)lv)*(evo?1.4f:1.0f);
      for(hh=0;hh<hops;hh++){
        int bestj=-1; float bd2=180.0f*180.0f;
        for(j=0;j<MAXENE;j++){ if(!g_ene[j].active||g_ene[j].spawn>0.0f) continue; int s=0,u; for(u=0;u<nv;u++) if(vis[u]==j){s=1;break;} if(s)continue;
          float dx=g_ene[j].pos.x-cx, dy=g_ene[j].pos.y-cy, d2=dx*dx+dy*dy; if(d2<bd2){ bd2=d2; bestj=j; } }
        if(bestj<0) break;
        if(nv<18) vis[nv++]=bestj;
        Enemy*e=&g_ene[bestj]; e->hp-=dm; e->flash=0.06f;
        spawn_ring(e->pos.x,e->pos.y,ene_radius(e->type)+4.0f,200.0f,0.16f,0.5f,0.9f,1.0f);
        burst(e->pos.x,e->pos.y,4,160.0f,0.5f,0.9f,1.0f);
        cx=e->pos.x; cy=e->pos.y;
        if(e->hp<=0.0f) kill_enemy(bestj);
      }
      if(nv>0) snd_play(SFX_HIT);
    }
  }
  if(g_weapLvl[W_DRONE]){ /* DRONES: 공전 자동사격 드론 (spawn_minib 재활용) */
    int lv=g_weapLvl[W_DRONE], evo=g_weapEvo[W_DRONE], n=1+lv/2+(evo?1:0), k; if(n>5)n=5;
    g_weapTimer[W_DRONE]-=dt;
    if(g_weapTimer[W_DRONE]<=0.0f){
      float cd=(0.5f-0.03f*(float)lv)*(evo?0.6f:1.0f); if(cd<0.15f)cd=0.15f; g_weapTimer[W_DRONE]=cd;
      float R=46.0f;
      for(k=0;k<n;k++){ float a=g_orbAng*0.7f+(float)k*6.2831853f/(float)n;
        float dx2=g_player.pos.x+f_cos(a)*R, dy2=g_player.pos.y+f_sin(a)*R;
        int ti=acquire_target(dx2,dy2,520.0f);
        if(ti>=0){ float tx=g_ene[ti].pos.x-dx2, ty=g_ene[ti].pos.y-dy2, l=f_sqrt(tx*tx+ty*ty); if(l<0.01f)l=1.0f;
          spawn_minib(dx2,dy2,tx/l,ty/l,BUL_DMG*(0.5f+0.15f*(float)lv)*(evo?1.3f:1.0f)); } }
      snd_play(SFX_SHOOT);
    }
  }
}
/* 보스 웨이브 (§3.6): 2분마다 본편 보스 재활용, 아레나 중앙 스폰, 시간 HP 스케일 */
static void oc_boss_waves(float dt){
  if(g_boss.active) return;
  g_ocBossT-=dt;
  if(g_ocBossT<=0.0f){
    g_ocBossT=120.0f; g_ocBossCount++;
    int type=(g_ocBossCount-1)%3;
    boss_setup_bounds(g_arenaX0+TILEF,g_arenaX1-TILEF,g_arenaY0+TILEF,g_arenaY1-TILEF);
    float span=(g_arenaX1-g_arenaX0)>(g_arenaY1-g_arenaY0)?(g_arenaX1-g_arenaX0):(g_arenaY1-g_arenaY0); /* DESCENT와 동일: max(w,h) */
    boss_spawn(type,g_ocBossCount,1.0f+0.3f*(float)(g_ocBossCount-1),0.5f*span);
    set_xmit(g_xmitBoss[type]); snd_play(SFX_PHASE); add_trauma(0.4f);
    /* 엘리트 웨이브 (§3.5): 보스와 함께 어픽스 엘리트 일괄 스폰 */
    int ne=4+g_ocBossCount; if(ne>8)ne=8; int ew;
    for(ew=0;ew<ne;ew++){ int ei=oc_spawn_at_edge(oc_pick_type());
      if(ei>=0 && g_ene[ei].type!=4 && !g_ene[ei].affix){
        int b1=(int)(xrnd()%5); g_ene[ei].affix=(unsigned char)(1<<b1);
        if(g_ene[ei].affix&1) g_ene[ei].eshield=1;
        g_ene[ei].hp*=1.15f; g_ene[ei].maxhp=g_ene[ei].hp; } }
  }
}
/* 레벨업 드래프트 (§14.4): draw3 불가침, g_upgSel 인코딩 >=100 → 무기(id-100), 0..MOD_COMMON → 패시브 모듈 */
static void oc_draft(void){
  int k;
  g_upgSel[0]=g_upgSel[1]=g_upgSel[2]=-1;
  for(k=0;k<3;k++){
    int cand[WEAPN+MOD_COMMON], nc=0, i, q, dup;
    for(i=0;i<WEAPN;i++){ if(g_weapLvl[i]>=WEAP_CAP) continue; dup=0; for(q=0;q<k;q++) if(g_upgSel[q]==100+i) dup=1; if(!dup) cand[nc++]=100+i; }
    for(i=0;i<MOD_COMMON;i++){ dup=0; for(q=0;q<k;q++) if(g_upgSel[q]==i) dup=1; if(!dup) cand[nc++]=i; }
    if(nc<=0) continue;
    g_upgSel[k]=cand[(int)(xrnd()%(unsigned)nc)];
  }
}
static void oc_check_evo(void){ /* 진화 조건: 무기 max + 짝 패시브 보유 (§4.4) */
  int w; for(w=0;w<WEAPN;w++)
    if(!g_weapEvo[w] && g_weapLvl[w]>=WEAP_CAP && g_mod[g_weapMeta[w].evoReq]>0){
      g_weapEvo[w]=1; set_xmit("WEAPON EVOLVED.\nPOWER SURGES."); add_trauma(0.3f); }
}
static void oc_apply(int sel){
  if(sel>=100){ int wid=sel-100; if(wid>=0&&wid<WEAPN&&g_weapLvl[wid]<WEAP_CAP) g_weapLvl[wid]++; }
  else if(sel>=0&&sel<MOD_COMMON) g_mod[sel]++;
  oc_check_evo();
  snd_play(SFX_PICKUP);
  g_state=ST_PLAY;
}
static void oc_level_check(void){
  if(g_ocXP>=g_ocXPneed){
    g_ocXP-=g_ocXPneed; g_ocLevel++;
    g_ocXPneed=5+g_ocLevel*4+g_ocLevel*g_ocLevel;
    oc_draft(); g_upgCtx=1; g_state=ST_UPG; snd_play(SFX_PHASE);
  }
}
/* ================================================================================= */

