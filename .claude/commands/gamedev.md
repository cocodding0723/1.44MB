---
description: NEON DESCENT 개발팀 전체 파이프라인 실행 (PM→기획검토→보강→[개발∥디자인]→검증→피드백)
argument-hint: "<태스크, 예: M1 엔진 골격>"
---
태스크 "$ARGUMENTS"에 대해 개발팀 파이프라인을 가동한다.

권장: ultracode/Workflow로 `.claude/workflows/gamedev-pipeline.js` 를 `args:{ task:"$ARGUMENTS" }` 로 실행(결정론적 단계·병렬·피드백 루프 내장).
수동 실행 시 순서:
1. `pm-orchestrator` — 계획·분해
2. `design-reviewer` — 기획 red-team (CRIT 누락 0이어야 진행)
3. `design-augmenter` — 공백 보강(필요 시)
4. 병렬: `gameplay-engineer`(구현) ∥ `art-director`(PNG 목업·컨펌)
5. 병렬: `qa-playtester` + `code-reviewer-game` + `size-guardian`
6. CRIT/HIGH 발견 시 `gameplay-engineer`로 회귀(≤3회)

규칙: 용량(rules/10) 최우선, 비주얼은 PNG 컨펌(rules/50), no-CRT(rules/20) 준수.
