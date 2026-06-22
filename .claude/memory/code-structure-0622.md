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
- **OVERCLOCK 무기 추가**: `nd_data.inc` W_* 매크로+WEAPN++·**`g_weapMeta[]` 테이블 한 줄**(name/desc/evoReq 통합), `nd_entity.inc` oc_weapons_update 거동 블록·oc_draft 후보, `nd_render.inc` 시각화 블록. (메타 3배열→단일 WeapMeta 테이블, +512B.)
- **모듈 추가**: nd_data.inc M_* enum + g_modName/Desc[MODN] 배열 + combat 효과(g_mod[M_X] 체크). **커먼은 인덱스 0~17(MOD_COMMON) 연속**(g_modW 가중치 있음), **레어는 끝에 append**(18~MODN, 가중 균등 10·draw3 레어분기). draw3 레어개수=MODN-MOD_COMMON. 즉시효과는 apply_mod 분기. 레어 과중첩 캡은 draw3 레어분기에. (v1.6 CRYO/EXECUTE가 레어 append 예시.)

## 신규 콘텐츠 (확장 구조 첫 활용)
- **PHANTOM (적 type 7, v1.5, 2026-06-22)**: 순간이동 교란 적. 위 "적 추가" 플로우대로 g_eneStat[8] 1행 + enemy_update 분기 + draw_enemy 분기(배칭은 다이아 폴백) + pick_enemy_type(L7)/oc_pick_type(t180). rules/50 목업 승인 후 구현. +1KB(111,104B). 임시 해금 낮춰 양 모드 스폰 검증 후 정상 복원.

## 검증
원본 단일 game.c vs 분할본 빌드 = exe 동일(110,080). 적 테이블 통합 후 109,568(−512B). 무기메타 110,080. sapp 110,080. enemy_update 추출 110,080(byte-identical). PHANTOM 111,104. 양 모드·전 화면 안정성 확인.
