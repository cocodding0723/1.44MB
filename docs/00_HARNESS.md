# 00 · 개발 하네스 사용서 (NEON DESCENT Dev Studio)

> 수많은 에이전트를 **개발팀처럼** 굴리는 멀티에이전트 하네스. 설정은 `.claude/`, 설계는 `DESIGN.md`+`docs/`.
> 참조: pm-skills(red-team·pre-mortem·test-scenarios), karpathy(verify-loop), agentmemory(메모리), ECC(worktree·santa-loop·team-builder), codegraph/Understand-Anything(코드 이해).

## 1. 팀 (에이전트 8) — `.claude/agents/`
| 역할 | 에이전트 | 모델 |
|---|---|---|
| PM/오케스트레이터 | `pm-orchestrator` | opus |
| 기획 검토(red-team) | `design-reviewer` | opus |
| 기획 보강(공백 채움) | `design-augmenter` | opus |
| 개발(no-CRT C) | `gameplay-engineer` | opus |
| 디자인(PNG 컨펌) | `art-director` | sonnet |
| QA(플레이테스트) | `qa-playtester` | sonnet |
| 코드리뷰 | `code-reviewer-game` | opus |
| 용량 감시 | `size-guardian` | haiku |

## 2. 파이프라인
```
PM 계획 → 기획검토 → 기획보강 → [개발 ∥ 디자인] → [QA + 코드리뷰 + 용량] → 피드백 루프(≤3) → 디자인 PNG 컨펌
```
- 기획검토에서 **CRIT 누락 0** → 개발 착수.
- 비주얼은 **PNG 목업 사용자 컨펌**(rules/50) 후 구현.
- 검증에서 CRIT/HIGH → 개발 회귀(최대 3회), 초과 시 PM 에스컬레이션.
- **용량 FAIL → 빌드/머지 차단**(rules/10, 최우선).

## 3. 실행 방법 (3가지)
**(a) Workflow / ultracode (결정론적 전체 파이프라인)**
- 스크립트: `.claude/workflows/gamedev-pipeline.js`
- 실행: Workflow 도구로 `scriptPath` 지정 + `args:{ task:"M1 엔진 골격", areas:["engine"], maxFeedback:3 }`.
- ultracode 세션에서 그대로 동작(각 단계 = 커스텀 에이전트, 병렬·피드백 루프 내장).

**(b) Agent 직접 fan-out (수동 조율)**
- 독립 태스크면 한 메시지에 여러 `Agent`(subagent_type=해당 에이전트) 병렬 호출 → 결과 종합.

**(c) 슬래시 커맨드 (단계별)**
- `/gamedev <task>` 전체 · `/pm-plan` `/spec-review` `/spec-augment` `/dev` `/design-review` `/qa` `/code-review` `/size-check` `/design-confirm`.

## 4. 병렬 작업 (git worktree) — rules/40
- 사전: `git init`(게임을 자체 repo로).
- `powershell -File scripts/worktrees.ps1 -Areas dungeon,combat,audio` → 영역별 worktree + `.coord/{task,handoff,status}.md`.
- 독립 영역만 병렬(던전 ∥ 사운드 ∥ UI). 머지 전 코드리뷰+size-check.

## 5. 규칙 — `.claude/rules/`
`00-core`(파이프라인·LAW) · **`10-size-budget`(🔴1.44MB 최우선)** · `20-no-crt` · `30-code-quality`(verify-loop) · `40-git-flow` · `50-design-confirm`.

## 6. 훅 (자동 게이트) — `.claude/hooks/` + `settings.json`
- `size-budget-check.ps1` — 빌드/종료 시 exe 측정, **FAIL 차단**, 이력 기록.
- `no-crt-guard.ps1` — src 편집 시 금지 CRT 함수 경고.
- `design-confirm-gate.ps1` — 컨펌 대기 비주얼 경고.

## 7. 산출 문서 — `docs/`
`01_기획서` · `02_상세기획서`(=DESIGN.md) · `03_기능명세서`(F-xxx) · `04_스토리보드` · `05_검사기준서`(TC-xxx) · `design/`(목업+CONFIRMATIONS).
- F-기능 ↔ TC-검사 1:1 매핑 → QA가 막힘없이 검증.

## 8. 메모리 — `.claude/memory/`
`MEMORY.md`(인덱스) + `design-frozen`·`size-history`·`harness`. 설계 결정·용량 이력만 append.

## 9. 빠른 시작 (Quickstart)
```
1) git init            # 자체 repo (worktree·결정론 위해)
2) /gamedev "M1 엔진 골격"   # 또는 Workflow(gamedev-pipeline.js)
3) build.bat           # 빌드 + 용량 측정(매번)
4) 디자인 요소는 art-director가 PNG 목업 → 컨펌 받고 진행
```

## 10. 현재까지 진행 (스냅샷, 2026-06-09)
- ✅ **설계 동결:** `DESIGN.md` GDD v1.0 (25개 섹션, 전 시스템 수치·수식 확정).
- ✅ **문서 5종:** 기획서/상세기획서/기능명세서/스토리보드/검사기준서.
- ✅ **하네스:** 에이전트 8 · 규칙 6 · 훅 3 · settings.json · build.bat · scripts(mockup·worktrees) · Workflow 파이프라인 · 메모리 시드.
- ⏭ **다음:** M1 엔진 골격 구현(`src/`) — `/gamedev "M1"` 또는 gameplay-engineer 착수.
