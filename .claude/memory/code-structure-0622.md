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
- **적 추가**: `nd_data.inc`의 `g_eneStat[]`에 한 줄(r/spd/hp/색) + AI는 combat_update 적 루프에 분기 + draw_enemy/batched 렌더 분기 + pick_enemy_type 해금. (스탯 4함수 흩어짐 → 테이블 단일화 완료.)
- **OVERCLOCK 무기 추가**: `nd_data.inc` W_* 매크로+WEAPN++·g_weapName/Desc/g_evoReq 배열, `nd_entity.inc` oc_weapons_update 거동 블록·oc_draft 후보, `nd_render.inc` 시각화 블록.
- **모듈 추가**: g_modName/Desc/g_modW 배열(이미 테이블) + apply_mod/combat 효과.

## 검증
원본 단일 game.c vs 분할본 빌드 = exe 동일(110,080). 적 테이블 통합 후 109,568(−512B, 중복제거). 양 모드·전 화면 안정성 확인. 동작 변경 0.
