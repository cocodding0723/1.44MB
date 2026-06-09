---
description: 변경 코드 리뷰 (용량·no-CRT·정확성·결정론) (code-reviewer-game)
argument-hint: "<리뷰 대상, 예: 최근 변경 / src/combat.c>"
---
`code-reviewer-game` 에이전트로 "$ARGUMENTS"를 리뷰하라.
우선순위: 용량(Δsize·판정)[최우선] > no-CRT > 정확성 > 결정론 > 단순/외과. 근거(파일:라인) 기반. 용량 FAIL·no-CRT 위반 = CRIT, 머지 차단.
