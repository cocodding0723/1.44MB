---
name: expansion-AB-0613
description: 2026-06-13 A(비주얼 폴리시)+B(콘텐츠) 확장 — GDD v1.0 FROZEN 초과 승인 확장
metadata:
  type: project
---

# A+B 확장 (2026-06-13) — FROZEN GDD v1.0 넘는 승인된 확장

사용자가 1.44MB 대비 4.4% 여유를 보고 "스프라이트 추가/풍부화" 요청 → A(비주얼)+B(콘텐츠) 둘 다 승인.
ultracode 병렬 워크플로우로 5스트림 설계(design-augmenter) → 직렬 구현(game.c 단일 파일) → 인게임 캡처 컨펌(rules/50, 3순위=실제 렌더 스크린샷).

## 용량 궤적
65,536(검증세션) → A1 68,096 → A2 70,144 → A3 71,680 → B(modules) 73,728 → B(enemies) 76,288 → **B(NEXUS) 78,336**. 캡 1.44MB의 **5.3%**. A+B 합 +12.8KB.

## A — 비주얼 (✅ 사용자 승인)
- A1 적/플레이어 다층 스프라이트(draw_enemy): 글로우+회전플레이트+코어+상태큐. Enemy에 `maxhp` 필드 추가(저HP 점멸용, .bss). 어픽스 5종 글리프. 플레이어 짐벌 트라이프레임 링+조준 핍. **모든 애니값 g_time/f_sin 기반(결정론 안전), 색상 정체성 유지**.
- A2 보스 다층+페이즈 escalation: CORE 동심링+회전 아이리스, WARDEN 자이로+N각링+코너노드+세그먼트 테더. 페이즈 전환 juice(흰 플래시+이중링+burst).
- A3 셰이더리스 포스트FX(render_post): 비네트(에지 그라디언트 4쿼드)/스캔라인(3px)/색수차 프린지(LINE_LOOP 적·청 오프셋)/액션 헤이즈(가산 팰릿). 신규 글로벌 `g_optCRT`(일시정지 `C` 토글 100/50/0), `g_fringeFx`(시각 전용, 보스 페이즈/히트 트리거). **광과민: 모든 펄스 FX는 g_optFlash 게이트, 정적 FX는 g_optCRT만**. 사용자 요청으로 보스전 강도 약화(ambient fringe 제거·pulse 0.5·haze 0.01).

## B — 콘텐츠 (⏳ 컨펌 대기)
- **모듈 enum 재배치**(전제조건): 커먼 0-17, 레어 18-25, MODN=26, MOD_COMMON=18. 신규 글로벌 g_frenzy/g_siphonK. draw3 n=rare?8:18, rare?MOD_COMMON+i. 모든 하드코드 14/6/20 → 상수화.
- 신규 커먼 4: VENGEANCE(저HP=뎀↑)/KINETIC(이속=뎀↑)/FRENZY(홀드 연사램프, g_frenzy 캡+1.5)/SIPHON(N킬마다 BITS다발+광란너지, 회복아님). 레어 2: GLASSCANNON(+60%뎀/스택캡2, 피격당 +1뎀 — 무한생존 차단)/CHAINLIGHT(명중 시 인접1체 아크, 재귀없음 arc_chain()).
- 신규 적: **LANCER**(type5, L5, 핫핑크, 스토크→520돌진→회복 FSM via e->t2, 텔레그래프선)/**WEAVER**(type6, L6, 바이올렛, 궤도+g_boom 기뢰 재활용 fuse 1.4s). AI 루프 최종 else→`else if(type==2)`로 변경(리코셰).
- 신규 보스: **THE NEXUS**(type2, L9): 로테이션 (kk-1)%3 → CORE(d3)/WARDEN(d6)/NEXUS(d9). 3노드 회전 삼각 빔케이지(beam_hit 3변), ph2 노드 방사링, ph3 수축+본체링, 터릿 소환. 신규 struct 필드 0(B->segAng/shrink/t1/t2/t3 재활용). WARDEN의 `else`→`else if(type==1)` (boss_update+render 양쪽).

## 함정/교훈
- **디버그 보스 스폰 시 본체 안 보임** = boss_update가 `g_grid[g_downGY][g_downGX]` 보스방 경계로 B->pos 클램프 → 디버그가 그 방을 안 맞추면 화면 밖. 디버그로 보스 확인 시 플레이어+보스를 다운링크방 중심으로 텔레포트해야 함. (NEXUS 코드는 정상)
- burst()는 fxsym 사용(검증세션에서 변경)이라 보스 페이즈 burst가 sim 시드 비오염.

## v1.3a QA 밸런스 패스 (2026-06-13)
5차원 병렬 QA(39발견, HIGH 8) 후 튜닝. **교훈: `hurt_player`가 진입부 `if(g_pIfr>0||g_dashT>0)return`로 self-i-frame 가드 → 같은 틱 다중 피격 불가.** QA가 "NEXUS 5HP/틱 즉사"를 HIGH로 올렸으나 이 가드를 놓친 오판 → 무효. 적대적 검증도 self-guard 놓칠 수 있으니 hurt_player 호출 다중성 판단 시 진입 가드 먼저 확인.
- 적용: NEXUS 밀도 완화(노드링 8발/3.6s·본체 12발·수축 12px/s·halfw 7고정·터릿 캡2/8s/클램프·SFX_PHASE), LANCER 코너진동(±0.6r)·예측선 0.6s, WEAVER 벽 스터터 damp, GLASS 3스택 캡, SIPHON 너지 FRENZY 가드, MAXPICK 256, KINETIC/VENGEANCE 버프. → 78,848B.
- 문서 동기화: DESIGN §12 표(L5 LANCER/L6 WEAVER/L9 NEXUS/L12 CORE 순환)·§20 bullets[256]·§25 노트·§26 v1.3a, docs/01·03(18/8/3/6), docs/05 TC-BOSS-01 수정+TC-ENM-06/07·TC-BOSS-05·TC-MOD-06/07 추가.

## 커밋됨 (origin/master)
- 533be39 feat(M4-M6+A+B 코드+DESIGN §26) / aea3e45 docs / a5f9654 chore. v1.3a QA 수정은 별도 커밋 예정.

## 미해결
- B 비주얼 사용자 컨펌 대기(CONFIRMATIONS.md ⏳). A는 ✅.
