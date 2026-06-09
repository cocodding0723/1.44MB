# NEON DESCENT — 프로젝트 하네스 (CLAUDE.md)

> 1.44MB Game Contest 2026 출품작. **no-CRT + Win32 + OpenGL** 네온 벡터 액션 로그라이크.
> 이 파일은 매 세션 자동 로드되는 팀 운영 규칙이다. 상세 규칙은 `.claude/rules/`, 설계는 `docs/`.

## 🔴 절대 불변 제약 (위반 = 실격)
1. **용량: 최종 standalone .exe ≤ 1,474,560 바이트.** 게임성보다 우선하는 단 하나의 하드 제약. → `.claude/rules/10-size-budget.md`
2. **독립 실행파일.** 웹/런타임 의존 금지. **no-CRT** C. → `.claude/rules/20-no-crt.md`
3. **에셋 파일 0개.** 그래픽·사운드·맵·폰트 전부 코드 절차생성.

> 우선순위: **용량 사수 > 게임성(재미·완성도) > 그 외.** 둘이 충돌하면 용량이 이긴다.

## 📂 단일 진실원천 (SoT)
- **설계:** `DESIGN.md` (GDD v1.0 FROZEN) — 모든 수치·시스템의 기준
- **산출 문서:** `docs/01_기획서.md` `02_상세기획서.md` `03_기능명세서.md` `04_스토리보드.md` `05_검사기준서.md`
- **하네스 사용법:** `docs/00_HARNESS.md`

## 👥 개발팀 (에이전트 로스터)
| 역할 | 에이전트 | 책임 |
|---|---|---|
| PM | `pm-orchestrator` | 계획·분해·디스패치·트래킹·핸드오프 |
| 기획 검토 | `design-reviewer` | GDD red-team: 문제·모순·누락 적발 |
| 기획 보강 | `design-augmenter` | 공백·누락 데이터·상세 스펙 채우기 |
| 개발 | `gameplay-engineer` | no-CRT C 구현, 용량 인지 |
| 디자인 | `art-director` | 비주얼 검토 + **PNG 목업으로 사용자 컨펌** |
| QA | `qa-playtester` | 실제 빌드 플레이테스트, 버그 적발(검사기준서 기반) |
| 코드리뷰 | `code-reviewer-game` | 정확성 + 용량 + no-CRT 준수 |
| 용량 감시 | `size-guardian` | exe 측정·예산 강제·감축 제안 |

## 🔄 파이프라인 (개발팀처럼)
```
PM 계획 → 기획검토(red-team) → 기획보강(공백채움)
   → [개발 ∥ 디자인]  (git worktree 병렬)
   → QA 플레이테스트 + 코드리뷰 + 용량감시  (병렬)
   → 문제? → 피드백 루프 → 개발 재진행 (bounded)
   → 디자인 산출물(맵·캐릭터·UI) → PNG/JPG 사용자 컨펌 게이트 → 진행
```
- 실행: `/gamedev` (전체) · 단계별 `/pm-plan` `/spec-review` `/spec-augment` `/dev` `/design-review` `/qa` `/code-review` `/size-check`
- ultracode/Workflow: `.claude/workflows/gamedev-pipeline.js` (결정론적 오케스트레이션)
- 팀/병렬: `.claude/teams.md` + `.claude/rules/40-git-flow.md`

## 📜 규칙 (`.claude/rules/`)
- `00-core.md` 팀 운영·파이프라인·산출물 계약
- `10-size-budget.md` **🔴 1.44MB 용량 예산(최우선)**
- `20-no-crt.md` no-CRT C 제약
- `30-code-quality.md` 코드 품질(Karpathy verify-loop)
- `40-git-flow.md` worktree 병렬 작업
- `50-design-confirm.md` 디자인 이미지 컨펌 게이트

## 작업 원칙
- 모든 변경은 `DESIGN.md`/`docs/` 스펙에 추적 가능해야 한다(근거 없는 임의 변경 금지).
- 코드·기능 추가 시 **용량 영향을 먼저 추정**하고, 빌드 후 `size-guardian`으로 실측한다.
- 디자인 요소(맵·캐릭터·UI)는 **구현 전 PNG로 컨펌** 받는다.
- 결정론(시드)·고정 타임스텝 유지 → QA 재현성 확보.
