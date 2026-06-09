---
description: 기획을 red-team하여 문제·모순·누락 적발 (design-reviewer)
argument-hint: "<검토 대상, 예: 보스 시스템 / 전체>"
---
`design-reviewer` 에이전트로 "$ARGUMENTS"(기본: DESIGN.md 전체)를 red-team하라.
load-bearing 가정 추출 → steelman 후 공격 → "Fails if" → impact×likelihood×cheapness 정렬. CRIT 누락 있으면 개발 착수 불가 명시. (읽기 전용)
