# harness — 멀티에이전트 개발 하네스 구성

**2026-06-09 구축.** NEON DESCENT 개발팀 하네스(`1.44/.claude/` + `docs/`).

구성:
- 에이전트 8: pm-orchestrator, design-reviewer, design-augmenter, gameplay-engineer, art-director, qa-playtester, code-reviewer-game, size-guardian.
- 규칙 6: 00-core, **10-size-budget(🔴1.44MB 최우선)**, 20-no-crt, 30-code-quality, 40-git-flow, 50-design-confirm.
- 훅 3: size-budget-check(FAIL 차단), no-crt-guard, design-confirm-gate (+ settings.json).
- 실행: Workflow `.claude/workflows/gamedev-pipeline.js`(ultracode 호환), 슬래시 커맨드, Agent fan-out.
- 병렬: `scripts/worktrees.ps1`(git worktree). 빌드/측정: `build.bat`. 목업: `scripts/mockup.ps1`(PNG 컨펌).

파이프라인: PM→기획검토→보강→[개발∥디자인]→[QA+코드리뷰+용량]→피드백(≤3). 비주얼은 PNG 컨펌 게이트.

참조 레포 차용: pm-skills, karpathy, agentmemory, ECC(worktree/santa-loop/team-builder), codegraph/Understand-Anything. 관련: [[design-frozen]].
