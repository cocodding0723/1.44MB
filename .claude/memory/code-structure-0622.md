---
name: code-structure-0622
description: src/ 도메인 분할 구조(.inc 인클루드) + 적 스탯 테이블 — 확장 시 어디를 고치는지
metadata:
  type: project
---

# 코드 구조 (2026-06-22 리팩토링)

## 파일 구성 — 단일 TU 인클루드 분할 (game.c만 컴파일, .inc는 #include)
`build.bat`은 `src/*.c`만 컴파일 → **game.c만** 빌드되고 나머지 `.inc`는 game.c가 순서대로 `#include`. static 전역/함수 그대로(extern 없음). 추가 시 순서(정의→사용) 유지 필수.
- `game.c`(207): 헤더·코어(f_sqrt/f_sin/RNG/입력전역)·#include 7줄·ND_REC·toggle_fullscreen·WinMainCRTStartup(엔트리)
- `nd_data.inc`(405): 사운드·BGM·상수(#define)·전역·구조체·**g_eneStat 적 스탯 테이블**·ene_radius/speed/color
- `nd_meta.inc`(206): 3x5 폰트·draw_text·시질·서사 교신·코덱스·세이브·페르소나·엔딩·파티클 스포너
- `nd_world.inc`(300): 던전 생성·아레나·new_run·spawn_enemy·spawn_wave·WndProc·shop_ped
- `nd_entity.inc`(501): 보스(spawn/die/update)·player_update·apply_mod/hurt·kill/explode/arc·OVERCLOCK 시스템(oc_*)
- `nd_combat.inc`(467): combat_update·camera_update·렌더 프리미티브(circle/diamond/hex)
- `nd_render.inc`(515): draw_enemy·draw_enemy_batched·render_world·HUD 헬퍼
- `nd_screens.inc`(352): render_hud·oc_render_hud·dim/post·codex/ending/title/gameover/pause/upgrade

## 확장 가이드 (어디를 고치나)
- **적 추가**: `nd_data.inc`의 `g_eneStat[]`에 한 줄(r/spd/hp/색) + AI는 `nd_combat.inc`의 `enemy_update(i,dt)` 함수 내 타입 분기 + draw_enemy/batched 렌더 분기 + pick_enemy_type 해금. (스탯 4함수 흩어짐→테이블 단일화. 적 AI는 combat_update에서 enemy_update로 분리.)
- **OVERCLOCK 무기 추가**: `nd_data.inc` W_* 매크로+WEAPN++·**`g_weapMeta[]` 테이블 한 줄**(name/desc/evoReq, evoReq는 미사용 커먼 모듈 고유 페어) + `nd_entity.inc` oc_weapons_update 거동 블록 하나. **oc_draft/oc_check_evo/oc_apply/HUD/arsenal 전부 WEAPN 순회라 자동 편입**(수정 불필요). 렌더는 선택(지속 비주얼이면 nd_render.inc 블록, 펄스류는 spawn_ring 재활용으로 불필요). v1.7 SHOCKWAVE가 예시(거동 블록+메타 한 줄만, +512B).
- **모듈 추가**: nd_data.inc M_* enum + g_modName/Desc[MODN] 배열 + combat 효과(g_mod[M_X] 체크). **커먼은 인덱스 0~17(MOD_COMMON) 연속**(g_modW 가중치 있음), **레어는 끝에 append**(18~MODN, 가중 균등 10·draw3 레어분기). draw3 레어개수=MODN-MOD_COMMON. 즉시효과는 apply_mod 분기. 레어 과중첩 캡은 draw3 레어분기에. (v1.6 CRYO/EXECUTE가 레어 append 예시.)

## 신규 콘텐츠 (2026-06-22 세션, 확장 구조 활용) — 적 9종/모듈 29/무기 10
- **적 PHANTOM(type7, v1.5, L7/t180)**: 순간이동 교란. **HIVE(type8, v1.8, L8/t120)**: 소환 노드(3s SHARD, 적<80 캡). 둘 다 g_eneStat 1행+enemy_update 분기+draw_enemy 렌더(배칭 다이아 폴백)+해금. rules/50 목업 승인 후 구현. **주의: oc_pick_type types/w 로컬배열은 해금 티어 추가 시 크기 키워야**(8→10).
- **레어 모듈 CRYO/EXECUTE/PHASE EDGE(v1.6)**: draw3 레어개수=MODN-MOD_COMMON, 과중첩 캡은 draw3 레어분기. CRYO는 Enemy.slowT 필드 추가.
- **OC 무기 SHOCKWAVE/SCATTER(v1.7)**: g_weapMeta 1행+거동 블록, 나머지 WEAPN 순회 자동. 펄스=spawn_ring/탄막=spawn_minib 재활용(전용 렌더 불필요).
- 각 신규 콘텐츠: 임시 해금/부여 낮춰 양 모드 스폰·동작 검증 후 정상 복원 → code-reviewer-game(CRIT/HIGH 0) → 문서(DESIGN §26·검사기준·CONFIRMATIONS) 동기화.

- **신규 보스 추가(PULSAR, type3, v2.0)**: 6개 통합 지점 — boss_update `else if(type==3)` AI(spawn_ebul 패턴), nd_render boss 색+렌더 분기, boss_radius, oc_boss_waves 순환, g_xmitBoss[] 확장, HUD 보스명. **동결 DESCENT 격리 핵심**: DESCENT 보스는 `nd_combat.inc (kk-1)%3` 그대로 두고 OC만 `%4`로 → PULSAR는 OC 전용(DESCENT 보스방·g_finalBoss 엔딩 미접촉, boss_die OC return이 엔딩 분기보다 선행). **g_xmitBoss[type] 인덱스 = 보스타입 → 보스 추가 시 배열 확장 필수**(OOB 주의).

## 검증
리팩토링: 원본 game.c vs 분할 = exe 동일. enemy_update 추출 byte-identical(110,080). 콘텐츠 누적: 110,080→115,712(+5,632B, 캡 7.85%, GREEN). 양 모드·전 화면·신규 콘텐츠 안정성 확인. QA 통합 패스(CRIT/HIGH 0). 신규 보스 동결 DESCENT 격리 검증.
