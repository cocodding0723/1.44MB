---
description: 비주얼 검토 + PNG 목업 생성·컨펌 요청 (art-director)
argument-hint: "<디자인 요소, 예: 플레이어 실루엣 / 팰릿>"
---
`art-director` 에이전트로 "$ARGUMENTS"의 목업을 만들어라.
scripts/mockup.ps1(System.Drawing)로 PNG를 docs/design/<element>-vN.png에 생성 → 사용자에게 이미지로 제시·컨펌 요청 → CONFIRMATIONS.md 갱신. 승인 전 구현 금지(rules/50).
