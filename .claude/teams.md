# 팀 구성 (병렬 worktree 작업 단위)

에이전트를 팀으로 묶어 git worktree에서 병렬 가동한다. (→ `rules/40-git-flow.md`)

## 팀 정의
| 팀 | 멤버 | worktree | 산출 |
|---|---|---|---|
| **PLAN** | `design-reviewer` → `design-augmenter` | (메인) | 검토 리포트 + `docs/` 보강 |
| **BUILD-A** | `gameplay-engineer` | `../nd-<area>` | `src/` 기능 구현 |
| **BUILD-B** | `gameplay-engineer` | `../nd-<area2>` | 독립 영역 병렬 구현 |
| **ART** | `art-director` | `../nd-art` | `docs/design/*.png` 목업 |
| **VERIFY** | `qa-playtester` + `code-reviewer-game` + `size-guardian` | (읽기·빌드) | 버그/리뷰/용량 리포트 |

## 병렬 가동 방법
1. **Agent 도구 직접 fan-out:** 독립 태스크면 한 메시지에 여러 `Agent` 호출(`subagent_type`=해당 에이전트).
2. **Workflow(ultracode):** `.claude/workflows/gamedev-pipeline.js` — 결정론적 단계·병렬·피드백 루프.
3. **team-builder(ECC):** 인터랙티브 픽 → 병렬 스폰 → 합의 종합.

## 분리 규칙
- **독립 영역만 병렬**: 던전생성 ∥ 사운드 ∥ UI/HUD ∥ 아트. (의존 시 순차)
- 전투·적AI·모듈은 인터페이스(엔티티/충돌 API) 합의 후 분리.
- 단일 `src/game.c` 충돌 방지를 위해 초기 **모듈 분할**(`src/dungeon.c`, `src/combat.c`, `src/audio.c`, `src/render.c`, `src/ui.c`) 권장 → 팀별 파일 격리.

## 모델 라우팅
PM/리뷰/엔지니어 = opus · ART/QA = sonnet · size-guardian = haiku. 대량 fan-out 시 격하.
