---
name: backlog-closure-0723
description: 2026-07-23 세션 — 이전 "남은 작업" 5건 정리(엔딩캡처 오기재 발견·pwsh 훅 복구·상점질문 종결·PULSAR 정적검증·SDD changelog 갱신). 개발 환경이 macOS라 Windows/GL 실캡처 불가한 제약과 그 대응법을 기록.
metadata:
  type: project
---

## 배경
2026-07-15 커밋(`22fbe7f`) 이후 처음 재개된 세션. 이 세션은 **macOS(darwin)** — `cl`(MSVC)·`pwsh`·`wine` 전무 확인. NEON DESCENT는 Win32+OpenGL no-CRT 게임이라 **이 환경에서 실제 exe 빌드/실행/인게임 스크린샷이 불가능**하다는 게 핵심 제약. → 향후 세션도 같은 환경이면 동일 제약 적용, "인게임 실캡처(rules/50 3순위)"는 Windows 세션에서만 가능.

## 처리한 항목
1. **pwsh 훅 부재 수정**: `.claude/settings.json`의 PostToolUse/Stop 훅이 전부 `pwsh -File ...` 호출인데 pwsh 미설치 → 매 Bash/Write/Edit 후 에러. `brew install powershell`(cask 아닌 core formula, dotnet 의존)로 해결, 3개 훅(size-budget-check/no-crt-guard/design-confirm-gate) 모두 exit 0 확인. **워크어라운드(guard-clause로 무음 처리)가 아니라 근본 설치로 해결** — 이 프로젝트는 용량/no-CRT 가드가 최우선 규칙이라 무음 비활성화는 부적절 판단.
2. **엔딩 스크린샷 오기재 발견**: `docs/design/shots-narr/ending-rot.png`가 실제로는 테알색 "ESCAPE" 화면(END_ESCAPE, src/nd_screens.inc:183)이었음 — 파일명과 내용 불일치. `ending-escape.png`로 정정(git mv). 실제 누락은 PURGE·THE ROT였음.
3. **PURGE/THE ROT 목업 생성**: 실캡처 불가 환경이라, `src/nd_meta.inc`의 실제 3×5 폰트 비트맵(`g_font`)·엔딩 색상·`draw_sigil` 로직을 파이썬으로 그대로 이식해 절차 생성(PNG 인코딩도 stdlib zlib만 사용, 외부 의존 0) → `docs/design/ending-purge-v1.png` / `ending-rot-v1.png`. **CONFIRMATIONS.md에 ⏳대기로 기록 — 사용자 시각 확인 전까지 확정 아님**(rules/50 면책 없음, 이 부분은 사용자 판단 없이는 종결 불가).
4. **상점 미해결 질문 4건 종결**: 코드(`nd_world.inc:92,100`, `nd_screens.inc:79`, `game.c:137`) 확인 결과 이미 확정 구현(제단 3개 고정·미니맵 $마커·구매키E·룸타이틀 없음) → 문서만 정합화, 코드 변경 없음.
5. **PULSAR P3 정적 검증**: 실기기 프레임타임 측정 불가 → 코드 기반 정상상태 탄수 계산(확장링+나선팔 ≈161발/MAXEBUL 256, 63% 여유)으로 오버플로 위험 낮음 확인. 체감/프레임타임은 여전히 실기기 검증 권장(안전성 아님, 손맛 문제라 코드로 대체 불가).
6. **docs/07 SDD changelog 갱신**: v2.1(2026-06-29 QA·밸런스·백로그) 행 추가 + 2026-06-22 리팩토링으로 change log의 src/game.c 라인번호 인용이 stale하다는 주의문 추가([[code-structure-0622]] 참조).

## 결론
- PURGE/THE ROT 엔딩 목업 **사용자 승인 완료(2026-07-23)** — CONFIRMATIONS.md ✅ 전환. 이번 세션 백로그 5건 전부 종결.
- 남은 선택사항(비차단): Windows 빌드 가능 세션에서 실제 인게임 캡처로 목업 교체하면 더 정확(rules/50 3순위).
