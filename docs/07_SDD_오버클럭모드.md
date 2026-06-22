# 07 · SDD — OVERCLOCK MODE (오토파이어 빌드-스노우볼 모드)

> **System Design Document.** NEON DESCENT에 VS(뱀파이어 서바이벌)/세피리아式 "**자동사격 + 레벨업 드래프트 + 빌드 폭발**" 호드 생존 모드를 **별도 모드**로 얹는 설계.
> 원칙: **본편(DESCENT 모드)을 건드리지 않고**, 엔진·렌더·모듈·적·보스를 최대 재활용. 하드 제약(≤1,474,560B · no-CRT · 결정론 · 에셋 0개) 전부 유지.
> 추적: 본 문서는 DESIGN.md(GDD) §26 확장 라인의 신규 모드 분기. 구현 시 DESIGN §26에 v1.4로 등재.

---

## 0. 요약 (TL;DR)

타이틀에서 모드 선택 → **OVERCLOCK**: 단일 절차 아레나에서 시간에 따라 밀려오는 기계 군세를 **무기가 자동 발사**되는 동안 버틴다. 처치 시 떨어지는 **데이터 조각(XP)**을 모아 **레벨업 → 3택1 강화**로 빌드를 굴리고, 오비탈·오라·노바 같은 **광역 무기**가 누적되며 화면을 쓸어버리는 파워 판타지로 진입한다. 2분마다 보스, 15분 생존이 목표(혹은 엔드리스). 죽으면 점수 정산.

- **재미 가설:** 능동 조준의 부담을 덜고(자동사격) **빌드 조합·생존 동선**에 집중 → "한 판 더" 루프. 기존 모듈 시스템이 그대로 VS의 "무기/패시브 드래프트"가 된다.
- **재활용율 추정(v0.3 현실화):** 코드 **~40% 재사용**. 순수 재활용은 엔진(창/GL/루프/입력/수학/RNG)·파티클/링/잔상·픽업/자력·사운드 신스·포스트FX·`draw_text_multi`(다줄/타이핑, 이미 구현)·세이브 시스템(magic+ver+FNV+LOCALAPPDATA+원자적쓰기, 이미 구현)·충돌 헬퍼·`explode_at`/`beam_hit`/`arc_chain` 동점안전 스캔. **신규/리팩터 필요:** 타게팅·XP/레벨·호드 스포너·광역무기 거동·모드 분기 **+ `draw_enemy_batched`(렌더러 신규)·`boss_spawn`+경계 인자화(본편 리팩터)·`oc_draft`/`oc_apply`(draw3 격리)·고정시드 훅·모드별 활성캡 분리.**
- **용량 추정(v0.3 현실화):** **+15~25KB** (현재 88,064B → 약 103~113KB, 캡의 ~7~8%). 여전히 무관(TARGET 64KB는 초과하나 HARD_CAP 1.44MB 대비 여유 막대). 세부 분해는 §8.

---

## 1. 설계 결정 (Design Decisions) — 권장안 + 대안

| # | 결정 | 권장안 | 대안 / 트레이드오프 |
|---|---|---|---|
| D1 | 본편 대체 vs 별도 모드 | **별도 모드** (타이틀 토글) | 대체는 완성된 본편 리스크↑·콘테스트 위험. 모드 분리가 안전 |
| D2 | 맵 구조 | **단일 큰 아레나**(절차 생성 1방, 장애물 배치) | 다방 연결은 호드 페이싱과 충돌. 아레나가 VS 정석 |
| D3 | 조준 | **자동(최근접) + 마우스 수동 오버라이드** | 완전 자동은 무기 다양화로 보완. 수동 병행이 손맛 유지 |
| D4 | XP 통화 | **기존 BITS 픽업 재활용**(자력 흡인 그대로) → 레벨 XP | 별도 젬 풀 신설은 픽업 풀 중복. BITS 재활용이 0-비용 |
| D5 | 레벨업 강화 | **`draw3` 드래프트 재활용** + 모드 전용 무기/패시브 풀 | 본편 모듈 캡 해제·스택 상한 상향 필요 |
| D6 | 승리 조건 | **15분 생존(최종 보스 격파) + 엔드리스 계속** | 순수 엔드리스도 가능(점수 갱신만) |
| D7 | 적 풀 크기 | **MAXENE 128→256** (필요 시 512, .bss) | VS는 수백. 256으로 시작, 렌더 배칭으로 perf 확보 |
| D8 | 결정론 | **유지** — 스폰/드래프트/거동 `g_rng`, 비주얼 `g_rngFx` | 변경 없음(QA 재현성) |

> **열린 항목(사용자 확정 필요):** D6 승리 조건(15분 고정 vs 순수 엔드리스), 무기 진화(D-EVOLVE, §5.4) 채택 여부, 모드 잠금(본편 클리어 후 해금 여부).

---

## 2. 코어 루프

```
[타이틀] → 모드선택(DESCENT / OVERCLOCK)
  OVERCLOCK 진입 → 아레나 생성(시드) + 시작 무기 1
  ──────────── 런 루프 (고정 120Hz sim) ────────────
  · 호드 스포너: 시간 t에 따라 가장자리에서 적 스트림 방출(밀도·HP·종류 t-스케일)
  · 자동사격: 보유 무기들이 각자 쿨다운으로 자동 발동(최근접/지정 타게팅)
  · 플레이어: WASD 이동 + 대시(회피) + (옵션)마우스 수동조준 + 스킬(블링크/EMP)
  · 처치 → XP 조각 드롭(자력 흡인) → 누적 → 레벨업 임계 도달
       → sim 정지 + 3택1 드래프트(무기 신규/레벨업/패시브) → 재개
  · 2:00마다 엘리트 웨이브, 보스 출현(CORE/WARDEN/NEXUS + PULSAR(OC 전용) 순환 %4)
  · 피격 누적 → 사망 → 점수 정산(시간×킬×레벨×보스)
  목표: 15:00 생존(최종 보스) → 클리어 점수, 이후 엔드리스
```

- **인텐시티 커브:** 평온(0~1분) → 스웜(밀도↑) → 엘리트 웨이브(2분) → 보스 → 반복 escalating. 보스 처치 시 짧은 보상 구간(레벨업 1~2회분 XP 폭발).

---

## 3. 시스템 명세

### 3.1 모드 상태 & 분기
- 신규 전역 `g_mode` (0=DESCENT, 1=OVERCLOCK). 타이틀에서 좌우키/클릭으로 토글, Space로 진입.
- `new_run()`이 `g_mode`에 따라 분기: OVERCLOCK는 던전 생성 대신 `arena_init()`, XP/레벨/타이머 리셋.
- 기존 `ST_PLAY/ST_UPG/ST_PAUSE/ST_OVER` 상태 재활용. ST_UPG가 레벨업 드래프트를 겸한다(컨텍스트 플래그 `g_upgCtx`).
- 메인 루프 sim/렌더 분기: OVERCLOCK는 던전·다운링크·방잠금 로직을 건너뛰고 `arena_update()`/`horde_update()`/`weapons_update()` 호출.

### 3.2 아레나 생성 (`arena_init`)
- 단일 큰 방(예 40×30 타일) + 절차 장애물(기둥·크레이트 — 기존 `obst_fit` 재활용)로 동선 변화.
- 카메라는 기존 추적(데드존+리드) 재활용. 경계는 벽(기존 타일 충돌).
- 결정론: `g_master` 시드로 레이아웃·장애물 배치.

### 3.3 자동사격 & 타게팅 (`weapons_update`)
- 무기는 `Weapon{ type, level, cd, timer }` 전역 배열(예 `MAXWEAP=8`). 보유 무기마다 `timer-=dt`, ≤0이면 발동 후 쿨 리셋.
- **타게팅 헬퍼** `acquire_target(originX,originY,maxRange)` → 최근접 active 적 인덱스(없으면 -1). 단순 O(MAXENE) 스캔(기존 explode/chain 스캔과 동급 비용).
- 수동 오버라이드: 마우스 버튼 홀드 시 기본 사격 무기는 마우스 방향(기존 `combat_update` 발사 블록 재활용), 미입력 시 자동 최근접.
- 발사체는 기존 `g_bul`/`spawn_minib`/`spawn_ebul`(적용 안함) 재활용.

### 3.4 진행: XP · 레벨 · 드래프트
- **XP:** 적 처치 시 `spawn_pickup(x,y,2)` (kind=2=XP조각, 기존 0=BITS/1=모듈오브에 추가). 자력 흡인·접촉 획득 로직 재활용. 획득 시 `g_xp += xpval(enemyType)`.
- **레벨 임계:** `xpNeed(lv) = 5 + lv*4 + lv*lv` (완만한 2차). 레벨업 시 `g_level++`, 잔여 XP 이월.
- **드래프트:** 레벨업 → `g_state=ST_UPG`, `g_upgCtx=OVERCLOCK`. `draw3`를 모드 풀(§5)로 추첨. 카드 = {신규 무기 / 보유 무기 레벨업 / 패시브}. 본편 캡(`mod_capped`)은 이 모드에서 무시(스택 상한 상향, §5.5).
- **보스 보상:** 보스 처치 시 XP 대량 드롭 + 즉시 레벨업 1회 보장(레어 가중).

### 3.5 호드 스포너 (`horde_update`)
- 시간 t(초) 기반: `spawnInterval = max(0.15, 1.2 - t*0.004)` (점점 촘촘), 한 번에 1~3마리 가장자리(화면 밖 링) 스폰.
- **적 종류 게이팅(시간):** 0~1분 헌터/샤드(fodder), 1~3분 +터릿/리코셰/LANCER, 3~6분 +포크/WEAVER, 6분+ 엘리트 다수. 기존 6종 + 신규 fodder 1~2종(§6).
- **HP/속도 t-스케일:** `hpMul = 1 + t*0.02`, `spdMul=min(1.7, 1+t*0.006)`. (본편 §12와 별도 곡선.)
- **동시 적수 캡:** `min(MAXENE, 40 + t*0.5)`. 캡 도달 시 스폰 보류(풀 보호).
- 엘리트 웨이브: 120초 배수에 어픽스 엘리트 일괄 스폰.

### 3.6 보스
- 2:00/4:00/…마다 보스(CORE/WARDEN/NEXUS) **그대로 재활용** + **PULSAR(OC 전용 4번째, v2.0)** → `(count-1)%4` 순환 — 단 아레나 중앙 스폰, 방잠금 대신 "보스 활성 중 일반 스폰 감속". HP는 시간 스케일.
- **PULSAR(type 3)**: OC 전용 방사 탄막 보스. P1 확장 링 → P2 +나선 팔 → P3 이중 링·고속. spawn_ebul 재활용. **DESCENT 보스 로테이션(%3)·엔딩 불변**(OC만 %4). g_xmitBoss[4] 인트로.
- 보스전 BGM/색수차는 기존 포스트FX 재활용.

### 3.7 사망 · 점수
- `score = floor(t)*10 + g_kills*5 + g_level*200 + g_bossKills*1000`. 정산 화면 재활용(ST_OVER), 분해 항목만 모드별 교체.
- 최고기록: 기존 `g_bestScore`와 별도 `g_bestTime`(OVERCLOCK 생존시간) 추적.

---

## 4. 신규 무기 아키타입 (VS式 광역·빌드폭발)

> 전부 기존 드로우 프리미티브(`circle_fill/line`, `draw_beam`, `diamond_fill`, 파티클)와 풀 재활용. 결정론 비주얼은 `g_time`/`g_rngFx`.

| 무기 | 모티프(VS) | 거동 | 레벨업 스케일 | 재활용 |
|---|---|---|---|---|
| **PULSE**(기본총) | 매직완드 | 최근접 자동 단발 | +발수·+연사·+뎀 | `combat_update` 발사 |
| **ORBITERS** | 킹바이블 | 플레이어 공전 노드 N개, 접촉뎀 | +개수·+반경·+회전속·+뎀 | `diamond_fill`+원-원 충돌 |
| **AURA** | 갈릭 | 반경 R 지속 장판, 0.25s 틱뎀+넉백 | +반경·+틱뎀 | `circle_fill` 글로우 + 범위 스캔(explode_at 류) |
| **NOVA** | 라이트닝링 | 2.5s마다 최근접 위치 폭발(r) | +빈도·+r·+동시타겟 | `explode_at` 직접 재활용 |
| **BEAM** | 홀리워터X | 회전 스윕 레이저(플레이어 기준) | +길이·+폭·+개수(다중암) | 보스 `draw_beam`+`beam_hit` 재활용 |
| **SWARM** | 나이프 | 최근접들에게 유도탄 일제 발사 | +발수·+유도력 | 기존 HOMING 탄 재활용 |
| **ARC** | 라이트닝 | 명중 시 다중 점프(체인) | +점프수·+뎀비율 | 기존 `arc_chain` 확장(재귀 허용·홉수 캡) |
| **TURRET-DRONE** | (세피리아 소환) | 자동사격 배치 드론 1~N | +개수·+드론 연사 | 적 AI 반전 + `g_bul` |
| **SHOCKWAVE**(v1.7) | (충격파) | 2.2−0.12×lv초 주기 플레이어 중심 대반경 펄스, 광역뎀+강넉백(스페이싱) | +반경·+뎀·+빈도 | 범위 스캔 + `spawn_ring` 2겹(전용 렌더 불필요). 진화 짝 KINETIC |
| **SCATTER**(v1.7) | (산탄) | 1.6−0.08×lv초 주기 전방위 탄막 버스트(8+2×lv발), 무차별 군중 클리어 | +발수·+빈도 | `spawn_minib` 재활용(탄 자동 충돌·렌더). 진화 짝 BOUNCE |

- **패시브(passive):** 이속·최대HP·픽업범위(자력)·쿨다운감소·뎀증·크리·회복(희소)·XP증가. 기존 모듈 다수 그대로 패시브로 재사용(POWER/AGI/COOL/MAGNET/CRIT/RAPID/REGEN/AEGIS/HEART…).

### 4.4 무기 진화 (D-EVOLVE, 선택)
- VS 시그니처: **무기 max레벨 + 특정 패시브 보유 → 진화 super-weapon**(보스 처치 보상에서 제안).
- 예: ORBITERS(max)+COOL → **SAWBLADE HALO**(공전+사출), NOVA(max)+POWER → **MELTDOWN**(연쇄 대폭발).
- 구현비용 중간(+2~3KB). **Phase 2로 분리** 권장 — MVP 후 채택 판단.

### 4.5 스택/캡 정책
- OVERCLOCK은 무기/패시브를 VS처럼 깊게 스택(무기 max 8레벨, 패시브 무제한 누적이되 수치 캡으로 degenerate 방지).
- 기존 `mod_capped`는 DESCENT 전용. OVERCLOCK 드래프트는 무기 레벨 8캡 + 패시브 자체 수식 캡(예 이속 ×2.0, 쿨 0.2s 하한) 사용.
- **degenerate 가드(§7 정신 유지):** 무한 회복 루프 금지(회복 패시브 희소·캡), 화면 전체 영구 장판 금지(AURA 반경 캡), 발사체 폭주는 풀 캡(`MAXBUL`)+수명으로 자연 제한.

---

## 5. 데이터 모델 (신규)

```c
/* 신규 전역 (전부 .bss 또는 작은 정적 — 파일 크기 무관) */
static int   g_mode;                 /* 0 DESCENT / 1 OVERCLOCK */
static int   g_level, g_xp;          /* 레벨/누적 XP */
static float g_runT;                 /* 런 경과(초), sim 누적 */
static int   g_bestTime;             /* OVERCLOCK 최고 생존(초) */
#define MAXWEAP 8
typedef struct { unsigned char type, level; float timer; } Weapon;  /* 24B×8 = .bss */
static Weapon g_weap[MAXWEAP]; static int g_weapN;
static float g_orbAng;               /* 오비탈 공전 위상(g_time 파생도 가능) */
/* 풀 상향 */
#define MAXENE 256   /* 128→256 (호드) — .bss 증가, 파일 크기 무관 */
/* 픽업 kind 확장: 0 BITS, 1 모듈오브, 2 XP조각 */
```

- 모든 대형 배열은 0초기화 정적(.bss) → **파일 크기 미포함**(rules/10 §5). 초기화 테이블 박지 않음.
- 드래프트 선택지(`g_upgSel[3]`)·`draw3` 재활용, 풀(`MAXWEAP`) 추첨 로직만 추가.

---

## 6. 적 (호드용 추가)

- **기존 6종 재활용**(헌터/터릿/리코셰/포크/LANCER/WEAVER) — 스폰 곡선만 시간 기반.
- **신규 fodder 1~2종**(VS 잡몹 감성, 저HP·다수):
  - **MITE**(소형 직진 돌격, HP 6×t스케일, 떼지어 스폰) — 샤드 스프라이트 재활용.
  - (선택) **CRAWLER**(느린 탱크, 화면 압박) — 신규 또는 포크 변형.
- 보스 3종 그대로. 어픽스 엘리트 시스템 그대로(시간 게이팅).

---

## 7. UI / HUD (OVERCLOCK 전용 요소)

- **상단 XP 바**(레벨 진행) + **LV n** + **타이머 mm:ss** + 킬 수.
- 드래프트 카드: 기존 모듈방 카드 UI 재활용(무기 아이콘=절차 글리프, 레벨 표기 Lv→Lv+1).
- 빌드 요약(일시정지): 보유 무기/레벨·패시브 목록.
- 전부 3×5 폰트 + 즉시모드 쿼드/라인 재활용(신규 에셋 0).

---

## 8. 기술 · 성능 · 용량

- **렌더 perf(최대 리스크) — 배칭은 "재활용"이 아니라 신규 작업:** 현 `draw_enemy`는 적 1마리당 글로우 원/회전 프레임/코어/어픽스 배지로 **5~9개 독립 `glBegin/glEnd`**(src/game.c:1541~1652). 256적이면 프레임당 1,300~2,300 드로우콜 → 즉시모드에서 60fps 불가. 따라서 **`draw_enemy_batched` 신규 작성(+2~3KB)**: §14.7 참조. "타입별 단일 배치"는 렌더러 재작성이며, 본편 `draw_enemy`는 손대지 않고 OVERCLOCK 전용으로 신규 경로를 둔다.
- **sim perf:** O(N) 스캔이 무기 타게팅·오라·노바에서 반복 → 프레임당 적 위치 1회 그리드/캐시 고려(스트레치). MVP는 단순 스캔(활성캡 256이면 허용, §14.6). 타게팅 동점 결정론은 §14.2.
- **용량 예산(v0.3 현실화):** 고정시드 훅 ~0.1KB / 타게팅 ~0.3KB / XP·레벨·드래프트 분기 ~1.5KB / `oc_draft`+`oc_apply` ~2KB / 무기 8종 거동 ~6~9KB / 호드 스포너 ~1.5KB / `boss_spawn`+경계 인자화 ~1KB / **`draw_enemy_batched` 신규 ~2~3KB** / UI ~1.5KB / 모드 분기 ~1KB. **합 +15~25KB** → ~103~113KB(HARD_CAP의 ~7~8%). 진화(Phase2) +2~3KB.
- **no-CRT/결정론:** 신규 코드 전부 `f_sin/f_cos/f_sqrt/xrnd` 사용, sim 랜덤 `g_rng`/비주얼 `g_rngFx`, 고정 120Hz. `memset`은 자작(`#pragma function(memset)`, src/game.c:12) 확인 — 신규 풀 0초기화 안전. 고정시드 디버그 훅은 §14.1.
- **풀 메모리:** 컴파일 상수 `MAXENE`는 **256으로 상향하되 본편 활성캡 128 / OVERCLOCK 활성캡 256으로 런타임 분리**(§14.6). Enemy 64B×256≈16KB .bss → exe 무영향, 런타임 RAM만. MAXBUL/MAXEBUL 256 유지.

---

## 9. 마일스톤 (구현 단계)

| MS | 산출 | 수용 기준 |
|---|---|---|
| **O1 MVP** | 모드 분기·아레나·자동사격(PULSE)·XP/레벨/드래프트·호드 스포너·시간 스케일·1 보스 | OVERCLOCK 진입→5분 생존 가능, 레벨업 드래프트 동작, 빌드 1 루프 성립, exe<+10KB |
| **O2 무기 세트** | ORBITERS·AURA·NOVA·BEAM·SWARM·ARC·DRONE + fodder MITE | 무기 7+종 드래프트, 화면 클리어 빌드 성립, 256적 60fps |
| **O3 폴리시** | 진화(선택)·밸런스 곡선·인텐시티 페이싱·UI 폴리시·사운드 | 15분 생존 클리어 루프, degenerate 없음, 컨펌 게이트 |

- **병렬화:** O2 무기들은 서로 독립 → worktree 병렬 개발 가능. 적 fodder·UI도 독립.

---

## 10. 검증 기준 (QA 훅, 05_검사기준서 확장 예정)

- **TC-OC-01(CRIT)** 모드 분기: 타이틀 토글→OVERCLOCK 진입, 본편 미영향.
- **TC-OC-02(HIGH)** 자동사격: 무입력 시 최근접 자동 발동, 마우스 홀드 시 수동 우선.
- **TC-OC-03(HIGH)** XP/레벨: 처치 XP 흡인→임계→드래프트 3택, sim 정지/재개.
- **TC-OC-04(HIGH)** 호드 스케일: 시간↑ 밀도·HP↑, 적수 캡 준수(풀 오버플로 없음).
- **TC-OC-05(HIGH)** 빌드 폭발: 무기 누적→광역 클리어 성립(스노우볼).
- **TC-OC-06(MED)** 결정론: 동일 시드+동일 입력→동일 스폰/드래프트.
- **TC-OC-07(MED)** perf: 256적+다수탄 60fps 유지(드로우 배칭).
- **TC-OC-08(HIGH)** degenerate 가드: 무한회복·영구전체장판 없음, 풀 캡 동작.
- **TC-OC-SIZE(CRIT)** exe ≤ 1,474,560B.

---

## 11. 리스크 & 대응

| 리스크 | 영향 | 대응 |
|---|---|---|
| 모드 추가로 스코프 팽창 | 본편 안정성·일정 | **별도 모드·엔진 재활용**으로 격리. O1 MVP 먼저, 진화는 Phase2 |
| 256적 렌더 perf | 프레임 저하 | 타입별 단일 배치 드로우, 글로우 레이어 수 제한, 필요 시 적수 캡 하향 |
| 밸런스 스노우볼 폭주 | 난이도 붕괴 | 무기 8레벨 캡·패시브 수치 캡·풀 캡, 시간 HP곡선 추격 |
| 자동사격이 손맛 희석 | 재미 저하 | 수동 조준 병행 + 대시 회피 동선 강조 + 무기 선택의 능동성 |
| 두 모드 코드 분기 복잡 | 유지보수 | `g_mode` 단일 분기점, 공유 헬퍼 최대화, 모드 전용 함수 접두사(`oc_`) |

---

## 12. 본편과의 관계 / 결합도

- 공유: 엔진(창/GL/루프/입력/수학/RNG), 렌더 프리미티브·파티클·링·잔상, 충돌, 모듈 효과 수식, 적·보스 AI·스프라이트, 사운드 신스, 포스트FX, 폰트/HUD 헬퍼, `draw3` 드래프트, 픽업/자력.
- 분리: 던전 생성·방잠금·다운링크·레이어 하강(DESCENT 전용) ↔ 아레나·호드·XP/레벨·자동사격·무기풀(OVERCLOCK 전용).
- 단일 분기점 `g_mode`로 sim/렌더 상단에서 갈라 결합도 최소화.

---

## 14. 갭 보강 (design-augmenter, 2026-06-14) — red-team §13 8개 공백 메움

> red-team §13이 적발한 8개 공백을 **구현 가능 수준 + QA 검증 가능 수용기준**으로 채운다.
> 원칙: **본편(DESCENT) 동결 설계 불가침** — `g_mode` 단일 분기 + `oc_` 접두사 격리 유지. 코드 인용은 src/game.c 실측.

### 14.1 고정 시드 디버그 빌드 훅 (red-team CRIT, TC-OC-06 선결)
- **문제:** `new_run()`은 `QueryPerformanceCounter`로 `g_master`를 매 런 다르게 설정(src/game.c:681, 부트 시 2248). 동일 시드 재현 경로가 코드에 없음.
- **설계 — 컴파일 상수 + 환경변수 2경로, 본편 동작 불변:**
  ```c
  /* new_run() 진입부, g_master 확정 직후에 디버그 오버라이드만 추가 */
  #ifdef ND_FIXED_SEED
    g_master = (ND_FIXED_SEED) | 1u;              /* 컴파일 상수 주입 (CI/QA 빌드) */
  #else
    { char sb[16]; DWORD sl=GetEnvironmentVariableA("ND_SEED",sb,sizeof(sb));
      if(sl>0&&sl<16){ unsigned int v=0,i; for(i=0;i<sl;i++){ if(sb[i]<'0'||sb[i]>'9')break; v=v*10u+(unsigned)(sb[i]-'0'); }
        if(v){ g_master=v|1u; } } }     /* 환경변수 ND_SEED 주입 (런타임 QA, 자작 atoi) */
  #endif
  ```
  - 릴리스 빌드(매크로 미정의 + `ND_SEED` 미설정)는 **기존 QPC 경로 그대로** → 본편·릴리스 동작 100% 불변. no-CRT 준수(`GetEnvironmentVariableA` 이미 세이브에서 사용, atoi 자작).
  - `g_master` 확정 후 모든 파생(`g_rng=g_master^…`, `g_rngFx`)이 결정되므로 양 모드 공통 적용. OVERCLOCK은 추가로 `g_runT=0`에서 동일 입력 시퀀스 → 동일 호드/드래프트.
- **수용기준(TC-OC-06 보강):** `cl /D ND_FIXED_SEED=12345` 빌드 2회 실행 + 동일 입력 로그 재생 시 (스폰 좌표·타입·드래프트 3선택지·보스 출현 프레임)이 **bit-identical**. 환경변수 `set ND_SEED=12345` 경로도 동일 결과.

### 14.2 acquire_target 동점 타이브레이크 (red-team CRIT, 결정론)
- **문제:** "최근접"만 명세 → 동거리 다수일 때 순서 미정 = sim 분기 위험.
- **설계 — `explode_at`/`arc_chain`의 인덱스순 스캔 패턴과 동일하게 명문화:**
  ```c
  static int acquire_target(float ox,float oy,float maxR){
    int best=-1; float bd2=maxR*maxR;          /* maxR<=0 → 전체사거리: bd2=매우 큰 값 */
    int j; for(j=0;j<g_eneCap;j++){             /* g_eneCap = 활성캡(§14.6) */
      Enemy*e=&g_ene[j];
      if(!e->active||e->spawn>0.0f) continue;   /* 스폰 텔레그래프 중 제외(explode_at과 동일 가드) */
      float dx=e->pos.x-ox, dy=e->pos.y-oy, d2=dx*dx+dy*dy;
      if(d2<bd2){ bd2=d2; best=j; }              /* strict < : 동점이면 갱신 안 함 → 최저 인덱스 우선 */
    }
    return best;                                 /* 없으면 -1 */
  }
  ```
  - **strict `<`** 이므로 거리가 정확히 같으면 **먼저 만난(=낮은 인덱스)** 적이 유지된다. 부동소수 비교는 `d2`(제곱거리)로 통일 → `f_sqrt` 호출 0, 비교 오차/순서 일관.
  - 동일 규약을 `oc_aura_tick`/`oc_nova`(범위 스캔)에도 적용: 인덱스 오름차순 1패스, strict 비교.
- **수용기준:** 동일 시드+동일 프레임에서 두 적이 동거리일 때, 타겟은 항상 낮은 인덱스. TC-OC-06에 포함.

### 14.3 boss_spawn() 추출 + 아레나 경계 인자화 (red-team CRIT, 본편 리팩터 +1~2KB)
- **문제:** ① 보스 스폰이 `combat_update` 인라인(src/game.c:1443~1458). ② `boss_update` 경계가 `Cell*c=&g_grid[g_downGY][g_downGX]`에 하드결합(src/game.c:950). 아레나는 그리드 셀이 아님.
- **설계 A — 경계를 전역 인자로 추출 (양 모드 공유):**
  ```c
  static float g_bossMnx,g_bossMxx,g_bossMny,g_bossMxy,g_bossCx,g_bossCy;  /* 보스 활동 경계+중심 (.bss) */
  ```
  - `boss_update` 도입부의 `Cell*c=&g_grid[...]; float mnx=(c->rx+1)*TILEF+br; …` 5줄을 **전역 사용으로 치환**:
    `float mnx=g_bossMnx+br, mxx=g_bossMxx-br, mny=g_bossMny+br, mxy=g_bossMxy-br; float rcx=g_bossCx, rcy=g_bossCy;`
  - 본편 보스 활성화 직후(현 인라인 스폰 블록) `g_bossMnx=(c->rx+1)*TILEF;` … 로 셀에서 채워 넣음 → **본편 거동 수치 동일**(회귀 0). §10에 동일 셀 산식 유지.
- **설계 B — `boss_spawn(cx,cy)` 추출:** 현 인라인 1446~1457(타입/HP/위상/타이머/ldir 초기화 + pos 중앙)을 `oc_*`가 호출 가능한 함수로 분리. 본편은 `boss_spawn` 호출 + 방잠금(`g_locked`)만 인라인 유지. OVERCLOCK은 `g_bossMnx..` 를 아레나 벽으로 채운 뒤 `boss_spawn(arenaCx,arenaCy)` 호출, 방잠금 없이 "보스 활성 중 일반 스폰 감속"만.
- **본편 회귀 QA(필수):** RT-BOSS-01 — d3/d6/d9/d12 보스전에서 경계 클램프·중앙 스폰·HP·위상전환이 리팩터 전후 동일(고정시드 §14.1로 재현). 통과 전 머지 금지.
- **비용:** +1~2KB(전역 6개 + 함수 분리 오버헤드). Δsize 실측은 size-guardian.

### 14.4 oc_draft / oc_apply_weapon — draw3 불가침 (red-team HIGH)
- **문제:** `draw3`(src/game.c:245)/`mod_capped`(237)은 **본편 26모듈 배열(`g_mod[]`) 전용**. OVERCLOCK 무기(`g_weap[]`)는 추첨 불가. `mod_capped` 우회를 draw3에 넣으면 본편 §7 가드 회귀.
- **설계 — 신규 `oc_draft`/`oc_apply`, draw3는 한 줄도 안 건드림:**
  ```c
  /* 추첨 후보 정의: 무기 10종(v1.7) + 패시브 N종, 각 카드 = {kind(0무기/1패시브), id, isNew} */
  static void oc_draft(void){            /* draw3 미사용 — g_upgSel[3]만 공유 재활용 */
    int k; g_upgSel[0]=g_upgSel[1]=g_upgSel[2]=-1;
    for(k=0;k<3;k++){
      /* 가중 후보 풀 구성: 보유 무기는 레벨업(레벨<8 캡), 미보유는 신규(g_weapN<MAXWEAP),
         패시브는 자체 수식 캡(§4.5) 미달 시 후보. 슬롯 간 중복 금지(g_upgSel[q] 검사, draw3와 동일). */
      /* xrnd()%total 누적 선택 (draw3와 동일한 결정론 패턴, g_rng 사용) */
    }
  }
  static void oc_apply(int sel){          /* 선택된 카드 적용 — apply_mod와 격리 */
    /* kind=0 무기: 보유면 g_weap[i].level++, 미보유면 g_weap[g_weapN++]={type,1,cd}.
       kind=1 패시브: oc_passive[id]++ (별도 배열, g_mod[] 미오염) + 수식 캡 적용. */
  }
  ```
  - **캡 격리:** OVERCLOCK 캡은 `oc_capped(card)`(무기 레벨 8 / 패시브 §4.5 수식 캡)로 **별도 함수**. `mod_capped`는 DESCENT 전용 그대로.
  - **레벨업 트리거:** `g_state=ST_UPG; g_upgCtx=1;` → ST_UPG 렌더/입력이 `g_upgCtx`로 분기(본편=draw3 결과, OC=oc_draft 결과). 적용은 `g_upgCtx`로 `apply_mod` vs `oc_apply` 선택.
- **수용기준(TC-OC-03 보강):** OVERCLOCK 드래프트 100회에서 `g_mod[]`(본편 모듈) 단 1바이트도 변하지 않음. 본편 모듈방 드래프트는 `oc_passive[]` 무관.

### 14.5 모드 × 트리거 sim정지 매트릭스 (red-team MED)
- **원칙(본편 N1 채택 확인):** 본편 서사(레이어/보스 인트로/엔딩 교신)는 **sim을 멈추지 않는 비차단 오버레이 + 표시전용 타이머(렌더 dt 누적, `g_xmitT`)** 방식 → sim 결정론 무영향. OVERCLOCK은 호드 페이싱을 끊으면 안 되므로 동일 원칙을 최대 적용.

| 트리거 | DESCENT | OVERCLOCK | 비고 |
|---|---|---|---|
| 레이어/존 진입 교신 | **정지 안 함**(오버레이, `g_xmitT` 렌더dt) | (해당 없음 — 단일 아레나) | N1 |
| 보스 인트로 교신 | **정지 안 함**(오버레이) | **정지 안 함**(오버레이) | 호드 지속, 보스 등장만 |
| 레벨업 드래프트 | (해당 없음) | **sim 정지**(ST_UPG, `g_upgCtx=1`) | 선택은 능동 결정 → 정지 정당. dt=0 |
| 일시정지(P/ESC) | **sim 정지**(ST_PAUSE) | **sim 정지**(ST_PAUSE) | 기존 |
| 사망 | **sim 정지**(ST_OVER) | **sim 정지**(ST_OVER) | 점수 정산 |
| 엔딩 | **정지 안 함→ST_OVER**(오버레이 후) | (엔드리스/15분 클리어 = ST_OVER) | N1 |

- **핵심:** OVERCLOCK에서 **드래프트만 sim 정지**(VS 정석, 능동 선택). 그 외 서사/연출은 **오버레이로 호드를 끊지 않음**. ST_UPG 동안 `g_runT`·`horde_update` 동결 → 정지 구간이 결정론·점수에 영향 없음(정지 중 dt=0, 표시 타이머만 렌더 dt).
- **수용기준(TC-OC-06 연계):** 드래프트 정지/재개가 `g_runT`를 진행시키지 않음(정지 전후 `g_runT` 연속). 보스 인트로 중 호드 스폰 계속(밀도만 감속).

### 14.6 모드별 활성 적 캡 분리 (red-team HIGH)
- **문제:** `MAXENE`는 컴파일 상수(src/game.c:184 `#define MAXENE 128`). 256으로 올리면 **본편 던전도 256 풀 순회** → 매 프레임 O(N) 스캔(타게팅·충돌·EMP·explode) sim 비용 2배.
- **설계 — 컴파일 상수 256, 런타임 활성캡 분리:**
  ```c
  #define MAXENE 256                 /* 풀 상한(.bss, 파일 크기 무관) */
  static int g_eneCap = 128;         /* 런타임 활성 순회 상한 */
  /* new_run(): g_eneCap = (g_mode==1) ? MAXENE : 128; */
  ```
  - **모든 적 순회 루프의 `for(j=0;j<MAXENE;…)`를 `g_eneCap`으로 치환**(draw·충돌·타게팅·explode_at·arc_chain·EMP·스폰 빈슬롯 탐색). 본편(`g_mode=0`)은 `g_eneCap=128` → **순회 비용·동작 100% 동일**(인덱스 128~255는 항상 비활성).
  - .bss 16KB 증가만, exe 파일 크기 무영향(rules/10 §5).
- **수용기준:** 본편 런에서 동시 활성 적이 128을 초과 생성 시도되지 않음(스폰 가드 `na<g_eneCap`). OVERCLOCK은 256까지 채움. 본편 prof: 리팩터 전후 프레임타임 동일(±측정오차).

### 14.7 렌더 배칭 = 신규 작업 `draw_enemy_batched` (red-team CRIT, +2~3KB)
- **문제:** `draw_enemy`(src/game.c:1541~1652)는 타입별 글로우 원·회전 프레임·코어·어픽스 배지로 **마리당 5~9 `glBegin/glEnd`**. 256적 = 1,300~2,300 드로우콜/프레임 → 즉시모드 60fps 불가. "재활용" 불가.
- **설계 — OVERCLOCK 전용 신규 경로, 본편 `draw_enemy` 불가침:**
  - **1패스 1배치 원칙:** 활성 적을 타입별로 모으지 않고, **단일 `glBegin(GL_QUADS)` … `glEnd()` 안에서 전 적의 코어 쿼드를 정점으로 누적**(파티클 배치와 동일 기법). 글로우는 별도 1배치(half-alpha 큰 쿼드). 어픽스 배지/회전 디테일은 **OVERCLOCK에서 생략 또는 엘리트만**(perf 우선, rules/10·게임필 절충).
    ```c
    static void draw_enemy_batched(void){          /* g_mode==1 전용 */
      glBegin(GL_QUADS);                            /* 글로우 패스: 타입색 half-alpha 쿼드 1배치 */
      int j; for(j=0;j<g_eneCap;j++){ Enemy*e=&g_ene[j]; if(!e->active||e->spawn>0.0f) continue;
        float r=ene_radius(e->type)*1.6f; /* 타입색 lookup → 4정점 push */ }
      glEnd();
      glBegin(GL_QUADS);                            /* 코어 패스: 불투명 쿼드 1배치 */
      for(j=0;j<g_eneCap;j++){ /* 코어 r 쿼드 4정점 push, flash 시 흰색 */ } glEnd();
      /* 엘리트 어픽스 배지만 소수 개별 드로우(상한 ~16) */
    }
    ```
  - 드로우콜 256적 기준 **~2 배치 + 엘리트 N**으로 축소(2,300 → ~수십). 색은 `glColor`를 정점별이 아닌 배치별로 묶기 위해 타입 분리가 필요하면 **타입당 2배치(최대 8타입×2=16배치)**로도 허용(여전히 2,300→~16).
  - 셰이크/글로우/팰릿은 유지(월드 매트릭스 동일). 스폰 텔레그래프 적은 별도 소수 드로우.
- **perf 목표·측정:** 256적+200탄 @720p **60fps(16.6ms) 유지**. 측정: 고정시드(§14.1) 15분 런에서 `QueryPerformanceCounter`로 프레임타임 p99 < 16.6ms 로깅(디버그 빌드). TC-OC-07이 이 수치를 검증.
- **art-director 플래그(rules/50):** 배칭으로 적 비주얼 디테일(회전 프레임·트윈팽 등)이 본편 대비 단순화됨 → **OVERCLOCK 적 룩 목업 PNG 컨펌 필요**. 디테일 생략 범위를 비주얼로 확정.

### 14.8 이미 닫힌 갭 — 실제 재활용 사례 (red-team이 신규로 오인)
- **(e) 다줄 텍스트:** `draw_text_multi(x,y,s,t,r,g,b,a,cut)` **이미 구현**(src/game.c:412~419). `\n` 분할 + `cut>=0` 시 타이핑 cutoff 지원. 보스 인트로·교신·코덱스에 사용 중(2037/2095/2110). **OVERCLOCK은 그대로 재활용** — 신규 작업 0. (소문자/`,!?'` 글리프 지원 여부는 `draw_text` 글리프 테이블에 의존 — 본편 교신문이 대문자+`.`,`\n` 위주이므로 OC HUD/카드도 동일 문자셋 사용 권장. 신규 문자 필요 시에만 글리프 추가, 별도 갭 아님.)
- **(f) 세이브:** `save_load`/`save_write`(src/game.c:466~487) **이미 구현**: `SAVE_MAGIC 0x4E44524E`+`SAVE_VER 1`+FNV-1a `save_sum`(459) 체크섬 + `GetEnvironmentVariableA("LOCALAPPDATA")`(461, shell32 미링크) + 원자적 `temp→MoveFileExA(REPLACE|WRITE_THROUGH)`(486). `g_bestTime` 필드 슬롯(f[6]) 이미 존재. **OVERCLOCK 최고기록은 `g_bestTime` 그대로 재활용** — 세이브 구조 변경 0. (버전 마이그레이션: 신규 OC 필드 추가 시 `SAVE_VER 2`로 올리고 `save_load`에서 ver==1 읽으면 OC 필드 0으로 기본값 — 1줄 분기.)
- **memset:** 자작 `#pragma function(memset)`(src/game.c:12) 확인 → 신규 `g_weap`/`oc_passive` 풀 0초기화 안전(CRT 인입 없음).
- **결론:** red-team이 신규로 본 (e)/(f)는 **N1/N2 서사 작업에서 이미 구현됨** → OVERCLOCK은 검증된 코드를 재활용. 이 항목들은 +0KB.

---

## 13. 설계검토 결과 (design-reviewer red-team, 2026-06-14) — **갭 보강 완료, 재검토 권장**

> **게이트 갱신: 보강 완료 → design-reviewer 재검토 권장.** "엔진 ~70% 재활용" 가정의 상당 부분이 코드와 충돌했고, §0/§8을 **재활용 ~40%, +15~25KB**로 현실화. 본편 동결 설계를 건드리는 리팩터(boss_spawn·MAXENE 활성캡)는 회귀 QA 게이트(RT-BOSS-01·본편 prof)로 격리. 아래 8개 공백은 §14에서 구현 가능 수준으로 메움.

- **[CRIT→✅ §14.7] 렌더 배칭 비현실**: `draw_enemy`는 적 1마리당 5~9개 독립 `glBegin/glEnd`(src/game.c:1541~1652) 확인. "타입별 단일 배치"는 재활용 아님 → **`draw_enemy_batched` 신규(+2~3KB)**로 §8·§14.7 재기재. perf 목표 256적 60fps, 측정법 명시. 비주얼 단순화는 art-director 컨펌 플래그.
- **[CRIT→✅ §14.1] 결정론 시드 부재**: `g_master=QueryPerformanceCounter`(src/game.c:681/2248) 확인. **`ND_FIXED_SEED` 컴파일 상수 + `ND_SEED` 환경변수 2경로** 설계, 릴리스 동작 불변. TC-OC-06 수용기준 bit-identical로 강화.
- **[CRIT→✅ §14.2] 자동타게팅 동점 미정의**: **strict `<` + 최저 인덱스 우선**, 제곱거리(`d2`) 비교로 `f_sqrt` 0회. explode_at/arc_chain 인덱스순 패턴과 동일하게 명문화.
- **[CRIT→✅ §14.3] 보스 그리드 결합**: `boss_update` 경계 `g_grid[g_downGY][g_downGX]`(src/game.c:950) + 인라인 스폰(1443~1458) 확인. **경계 전역 인자화(`g_bossMnx..`) + `boss_spawn` 추출** 설계. 본편 회귀 RT-BOSS-01 게이트(+1~2KB).
- **[HIGH→✅ §14.4] draw3/apply_mod 모듈배열 종속**: `draw3`/`mod_capped`는 `g_mod[]` 전용 확인. **`oc_draft`/`oc_apply`/`oc_capped` 신규, draw3 불가침**. `g_upgCtx`로 ST_UPG 분기. 본편 §7 가드 격리(드래프트 100회 `g_mod[]` 불변 수용기준).
- **[HIGH→✅(이미구현) §14.8] no-CRT 세이브 함정**: `save_load`/`save_write`(466~487) **이미 구현 확인** — magic+ver+FNV `save_sum`+`GetEnvironmentVariableA(LOCALAPPDATA)`+원자적 `MoveFileExA`. `g_bestTime`(f[6]) 슬롯 존재. memset 자작 확인. OVERCLOCK 재활용 +0KB.
- **[HIGH→✅(이미구현) §14.8] draw_text 다줄 미지원**: `draw_text_multi`(412~419) **이미 구현 확인** — `\n` 분할 + 타이핑 cutoff. OVERCLOCK 재활용 +0KB.
- **[HIGH→✅ §14.6] MAXENE 공유**: `#define MAXENE 128`(src/game.c:184) 확인. **상수 256 + 런타임 활성캡(`g_eneCap` 본편128/OC256) 분리**, 적 순회 루프 전수 치환. 본편 동작·비용 불변(.bss만 증가).
- **[MED→✅ §14.5] sim정지 정책**: 모드×트리거 매트릭스 명문화. 본편 N1(비차단 오버레이) 원칙 적용 — OVERCLOCK은 **드래프트만 sim 정지**, 서사/보스인트로는 호드 비차단. 바이트 추정 §0/§8에서 +15~25KB로 하향 반영.

**→ 8개 공백 보강 완료(§14). O1 착수 전 design-reviewer 재검토 1회 권장**(특히 §14.3 boss_spawn 리팩터의 본편 회귀 범위, §14.7 배칭 비주얼 단순화 art-director 컨펌). 잘 설계된 부분(유지): 별도모드·g_mode 격리 전략, 팰릿=챕터 매핑(코드 일치 확인), explode_at/beam_hit/arc_chain 동점안전.

---

*v0.3 (2026-06-14). design-augmenter가 red-team §13 8개 공백 보강 완료(§14). 게이트 상태: 착수 보류 → **재검토 권장**. 다음: design-reviewer 재검토 1회 → 통과 시 O1 MVP 착수(고정시드 훅·oc_draft·boss_spawn 리팩터 우선). 비주얼(OVERCLOCK 적 룩 단순화 §14.7)은 rules/50 art-director 컨펌 게이트.*

---

## 15. Changelog

| 버전 | 날짜 | 작성 | 변경 |
|---|---|---|---|
| **O1 MVP 구현** | **2026-06-18** | **gameplay** | **착수→구현 완료.** `g_mode` 분기·아레나·자동사격(PULSE)·ORBITERS·NOVA·XP/레벨/드래프트(oc_draft/oc_apply, draw3 불가침)·호드 스포너·보스 웨이브(boss_spawn 추출 §14.3)·oc_render_hud. 본편 DESCENT 동결 보존(회귀 확인). +10KB → 99,840B(캡 6.8%). DESIGN §26 v1.4 등재. **남은 O2/O3**: 무기 5종·256적 배칭·진화·고정시드훅. |
| v0.1 | 2026-06-14 | (초안) | OVERCLOCK 모드 SDD 초안 |
| v0.2 | 2026-06-14 | design-reviewer | §13 red-team: 8개 공백 적발, **착수 보류**(재활용 ~70%→~40% 의심) |
| **v0.3** | 2026-06-14 | **design-augmenter** | **§14 신설(8개 공백 보강).** §0 재활용율 70%→**40%**, 용량 +8~14KB→**+15~25KB**(코드 실측 기반). §8 렌더 배칭을 "재활용"→**신규 작업(`draw_enemy_batched`)**으로 재기재, 바이트 분해 현실화. §13 게이트 **착수보류→재검토 권장**, 각 항목에 보강 위치(§14.x) 주석. 본편 동결 설계 불변(코드 변경 0, 문서만 갱신). 신규 수용기준: TC-OC-06 bit-identical(§14.1·2), RT-BOSS-01 본편 회귀(§14.3), 드래프트 100회 `g_mod[]` 불변(§14.4). |

> **changelog 근거 추적:** v0.3 모든 수치는 src/game.c 실측 인용(고정시드 681/2248, draw_enemy 1541~1652, boss_update 950, draw3/mod_capped 237~275, MAXENE 184, draw_text_multi 412, save 459~487, memset 12). 임의 변경 없음(rules/30 §3 외과적 변경).
