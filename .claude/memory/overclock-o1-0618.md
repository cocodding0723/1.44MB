---
name: overclock-o1-0618
description: 2026-06-18 OVERCLOCK MODE O1 MVP 구현 — VS式 호드 생존 별도 모드, 본편 DESCENT 동결 보존
metadata:
  type: project
---

# OVERCLOCK MODE O1 MVP (2026-06-18)

docs/07 SDD의 VS/세피리아式 자동사격 호드 생존 모드를 **별도 모드**로 구현(O1 MVP). 사용자 /goal "남은 작업 구현". 본편 DESCENT 동결 설계 보존 — `g_mode` 단일 분기 + `oc_` 접두사 격리. 빌드 99,840B(캡 6.8%, +10KB).

## 구현된 것 (단일 파일 src/game.c)
- **모드 분기**: 타이틀 `TAB`로 DESCENT/OVERCLOCK 토글(`g_mode`). `new_run` 분기(아레나 vs 던전). `combat_update`에서 OC 시스템 호출 후 DESCENT 방·다운링크 로직을 `if(g_mode==MODE_DESCENT){...}` 게이트로 격리.
- **아레나** `oc_arena_init`: 46×32 단일 방 + 기둥(tile 3) 엄폐. 경계 `g_arenaX0..Y1`.
- **호드** `oc_horde_update`: `interval=1.2-t*0.004`(최소 0.15), 활성캡 `40+t*0.5`(MAXENE-8 상한), `oc_pick_type` 시간게이팅(t<60 헌터/샤드, <180 +터릿/리코셰/랜서, 이후 +포크/위버). **g_depth를 t파생(`1+t/25`, 캡12)으로 두어 기존 HP/속도/엘리트 스케일 재활용**.
- **자동사격**: PULSE는 발사 블록 `g_mode==OC && !mouseDown` 시 `acquire_target`(최근접, strict< 동점 타이브레이크 §14.2)로 자동조준. 마우스 홀드=수동 우선. PULSE 레벨=연사/탄수 보너스. 신규 무기 7종=`oc_weapons_update`(전부 승인 프리미티브 재활용): ORBITERS·NOVA·AURA·SWARM·BEAM(회전 스윕 레이저 draw_beam/beam_hit)·ARC(체인 라이트닝 방문셋)·DRONES(공전 자동사격 드론). 무기 캡 8 = SDD MAXWEAP 충족.
- **밸런스(플레이테스트 발견)**: OC XP 픽업 자력 흡인 ×2.4 (방클리어 자동수거 없는 호드 모드라 기본 70px로는 레벨업 너무 느림). 픽업 loop에서 `g_mode==OC && kind==2`만 적용.
- **XP/레벨/드래프트**: 처치 시 XP조각(`spawn_pickup kind=2`, xc=타입별 1~3) 드롭→자력흡인→`g_ocXP+=2`→임계 `5+lv*4+lv²`→`oc_level_check`→ST_UPG+`g_upgCtx=1`. `oc_draft`(draw3 불가침, g_upgSel 인코딩 ≥100=무기·0..17=커먼모듈)/`oc_apply`. 무기캡 8.
- **보스 웨이브** `oc_boss_waves`: 120s마다 CORE/WARDEN/NEXUS 순환, 아레나 중앙. **`boss_spawn`/`boss_setup_bounds` 추출(§14.3) + 보스 경계 전역화(`g_bossMnx..g_bossCy`)** — boss_update top·DESCENT 인라인 스폰·WARDEN ph3 렌더가 전역 사용. 본편 거동 수치 동일(센터=(mnx+mxx)/2=룸센터, 회귀 0).
- **HUD** `oc_render_hud`: XP바·LV·타이머mm:ss·킬·보스HP바. 사망 OC점수 `floor(t)*10+kills*5+lv*200+boss*1000`·`g_bestTime` 갱신, 부패도 미증가(DESCENT 전용). 게임오버 OC 분해 화면.

## 리팩터 (사용자 요청 "분량 커지면 리팩토링 같이")
하트·피격비네트·교신오버레이·십자선을 공용 헬퍼 `draw_hearts`/`draw_hurt_vignette`/`draw_comms_overlay`/`draw_crosshair`로 추출 → `render_hud`(DESCENT)·`oc_render_hud` 공유(중복 제거). [[design-frozen]] 보존.

## 검증
인게임 캡처(docs/design/shots-oc): OC 타이틀 모드 표시, 아레나 진입(XP/LV/타이머 HUD), 자동사격 킬(11s에 KILLS 10·무수동발사), XP 흡인, 엘리트 스폰. DESCENT 회귀(shots-oc/descent-regress): HUD/던전/미니맵/교신 정상. 양 모드 크래시 0(13s 런).

## 추가 specced 기능 (2026-06-18, 리뷰 후 누락분 보강)
- **OC 엘리트 웨이브(SDD §3.5)**: oc_boss_waves에서 보스와 함께 어픽스 엘리트 4~8체. oc_spawn_at_edge가 인덱스 반환하도록 변경 → affix 강제 부여.
- **BGM P3 격화(§16.4)**: g_bgmIntense(boss_update ph==3 설정, boss_die 해제) → bgm_sample 템포 +10%(spb 22050→20045)+디튠(root×1.03). bgm_sample이 g_boss보다 앞 정의라 플래그로 디커플.
- **미니맵 Tab 확대(§18 스트레치)**: g_mapZoom(ST_PLAY Tab 토글), 미니맵 셀/스텝/마커 cs·step·ms 파라미터화 2×. 타이틀 Tab=모드토글과 비충돌(상태별).
- **F11 전체화면(§18·§19)**: toggle_fullscreen(WS_POPUP↔WS_OVERLAPPEDWINDOW+SetWindowPos 모니터크기). GL 컨텍스트 재생성 없음 → 매 프레임 GetClientRect+glViewport가 적응(저위험). F11 토글 2회 무크래시.
- 108,544B(7.4%). DESCENT·OVERCLOCK 안정성 확인.

## 빌드 요약 UI + 코드리뷰 (2026-06-18)
- **OC 일시정지 빌드 요약(§7)**: render_pause에 OVERCLOCK 시 "ARSENAL" 보유 무기 목록(이름+LV / *EVO*) 렌더. 기존 draw_text 재활용(비주얼 게이트 무관).
- **code-reviewer-game 리뷰 1회**: OVERCLOCK/상점/BGM/진화/시드훅 ~600줄. **CRIT/HIGH 0**, no-CRT/결정론(bgm_sample이 g_rng 미오염·메인스레드)/바운즈/boss_spawn 회귀/모드게이팅/용량 전부 PASS. MED/LOW 5건 전부 적용: ①OC 보스 ph3 shrink span max(w,h)로 ②g_finalBoss OC new_run 리셋 ③apply_mod g_upgCtx=0 ④DRONE 진화짝 M_RAPID→M_HOMING(BEAM과 중복 제거) ⑤oc_render_hud `h*0.0f+64` 정리. 107,008B(7.3%).

## 무기 진화 D-EVOLVE (§4.4) + 고정시드훅 (§14.1, 2026-06-18)
- **진화**: 무기 max(8)+짝 패시브(`g_evoReq` 페어링) → `g_weapEvo[w]=1` 자동(`oc_check_evo`, 드래프트 적용마다). 진화 시 oc_weapons_update에서 개수/반경/뎀/연사/관통 멀티플라이어. 렌더(BEAM/DRONE 개수)도 evo 반영. PULSE 진화=관통+2(발사 블록). SDD가 "Phase2 권장"했으나 specced §4.4라 목표("남은 작업") 범위로 구현. 승인 프리미티브 재활용→비주얼 게이트 무관. 60드래프트 장기런 무크래시.
- **고정시드훅**: new_run g_master 확정 직후 `#ifdef ND_FIXED_SEED`(CI 상수) / 환경변수 `ND_SEED`(자작 atoi) 2경로. 릴리스(미정의)는 QPC 불변. shell32 미링크라 GetEnvironmentVariableA 사용(세이브와 동일 패턴). 106,496B.

## 상점 (§13 DESCENT, 2026-06-18 — rules/50 ✅사용자 승인 후 구현)
art-director 목업(`scripts/shop-mockup.ps1`→`docs/design/shop-v1.png`) → 사용자 승인 → 구현. 방 type 4 신설. generate()에서 비보스 레이어 35%로 전투방(type1) 1개를 저수지 추첨해 상점으로 대체(g_shopGX/GY, g_shopPrice[3] +10%/L, g_shopBought[3]). 제단 3종 shop_ped(i): 모듈(draw3 3택1) 60 / 수리(+2HP) 45 / 리롤(랜덤 커먼) 30. 근접<45px + `E`(g_wantBuy 래치) + BITS충분 + 미구매 → 구매. 미니맵 `$` 앰버 마커(rt==4). 월드 라벨 draw_text scale6. 검증: 강제 100% 빌드로 배치/렌더/구매 경로 무크래시 확인(헤드리스라 비주얼 캡처는 목업으로 갈음). 105,472B(7.2%). CONFIRMATIONS.md S섹션 ✅.

## BGM (§16.4, 2026-06-18)
절차 BGM을 `snd_fill` 믹서에 추가(`bgm_sample()`, 게임상태 전역 이후 정의 — `snd_fill` 앞 전방선언). 120BPM, `pal_tier` 루트(`g_bgmRoot[5]`), 아르페지오(sine 플럭, 마이너펜타토닉 `g_arpScale`+`g_semi[13]` 2^(n/12) 테이블, 레이어 시드 변주)+베이스(saw)+킥(비트1·3). ST_PLAY/UPG/PAUSE만 재생. 일시정지 `B` 토글(`g_bgm`)+`M` 음소거. 양 모드 공용. 103,936B(7.0%). **사운드 품질은 사용자 청취 검증 필요**(이 환경 오디오 재생 불가). 교훈: 사운드 전역(g_state/g_depth)이 snd_fill보다 뒤 선언 → bgm_sample 전방선언으로 해결.

## QA 패스 (qa-playtester, 2026-06-18)
신규 v1.4 기능(OC/상점/BGM/진화/배칭) 검사기준서(F-OC/SHOP/BGM/OPT) 대조 + 코드분석(헤드리스라 비주얼 검증 불가). **행동 버그 4건 적발→수정**: ①[HIGH] g_bgmIntense가 new_run에서 미리셋→런 간 P3 BGM 누수 ②[MED] PULSE lv2 샷카운트 `(int)plv/2`=0(정수나눗셈)→`(int)((plv+1)/2)` 천장 ③[LOW] 상점 모듈 g_upgCtx(이미 apply_mod에서 0설정, 무해) ④[LOW] g_mapZoom new_run 미리셋. ①②④ 수정(new_run에 g_bgmIntense=0;g_mapZoom=0; + PULSE 식). 나머지 전 TC PASS(XP임계·점수식·호드캡·진화페어링·상점구매·모드격리). 검사기준서 TC-OC/SHOP/BGM/OPT 추가됨. 110,080B.

## 256적 배칭 렌더 (§14.6/§14.7, 2026-06-18 — rules/50 ✅승인)
art-director 목업(`scripts/batch-mockup.ps1`→`oc-batch-v1.png`) → 승인 → 구현. **g_eneCap 18루프 치환은 안 함**(MAXENE 256 올려도 DESCENT는 웨이브 캡상 미사용·256순회 무시수준 → 회귀 위험 회피). `draw_enemy_batched`(OC 전용): 패스당 단일 glBegin(글로우/삼각코어/사각다이아코어 + 엘리트·스폰 개별), ene_color 헬퍼(draw_enemy 색 일치). render_world에서 `g_mode==OC?draw_enemy_batched():per-enemy draw_enemy`. MAXENE 128→256으로 OC 호드 캡 자동 40+t*0.5→248. 45s 밀집 호드 무크래시. **60fps 실측은 실데스크톱 필요**(헤드리스 측정 불가). 110,080B(7.5%).

## 영상 녹화 인프라 + 헤드리스 캡처 교훈 (중요)
- `#ifdef ND_REC`(릴리스 미포함) glReadPixels 프레임 덤프(rec_frames.bin, 2x다운샘플·수직반전·SwapBuffers 前 백버퍼 읽기) + `scripts/record-oc.ps1`(PrintWindow 루프)·`assemble-rec.ps1`(raw→GIF)·`gif-from-stills.ps1`.
- **이 자동화/샌드박스 환경은 헤드리스 → GL 라이브 캡처 전부 실패**: PrintWindow=black GL client(DWM 미합성/occlusion), CopyFromScreen=VRAM garbage(실제 디스플레이 없음), glReadPixels=undefined(**OpenGL 픽셀 오너십**: 비가시/occluded 창의 프레임버퍼는 정의 안 됨). 스틸이 가끔 성공한 건 런치 직후 짧은 가시 구간 덕.
- **교훈**: 이 환경에서 GL 게임 영상 녹화 불가. 실제 데스크톱(가시 창)에서만 PrintWindow/glReadPixels 정상. 영상 필요 시 사용자 머신에서 `cl /D ND_REC` 빌드 → 구동 → ffmpeg로 인코딩(자작 GIF-LZW는 비균일 프레임 desync 잔존).
- pwsh7(.NET10)에서 System.Drawing.Common은 Add-Type 컴파일 불가(internal interface) → 런타임 사용만 가능. 컬렉션/Rectangle 등은 `-ReferencedAssemblies` 명시 필요.

## 미착수 (O2 잔여/O3)
BEAM/ARC/DRONE 무기, MAXENE 256+`draw_enemy_batched` 배칭(256적 60fps), 무기 진화(D-EVOLVE), 고정시드훅(§14.1, 현재 QPC시드), **OC 보스 인게임 실측**(120s 미도달 — math 등가로만 검증). [[size-history]] 100,864B.
