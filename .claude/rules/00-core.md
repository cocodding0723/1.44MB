# 00 — 팀 운영 · 파이프라인 · 산출물 계약 (코어 규칙)

## 파이프라인 단계 & 게이트
1. **PM 계획** (`pm-orchestrator`) — GDD/문서에서 작업을 마일스톤·태스크로 분해, 의존성·병렬성 식별, 핸드오프 작성.
2. **기획 검토** (`design-reviewer`) — load-bearing 가정을 red-team. 출력: `문제/모순/누락` 목록(impact×likelihood×cheapness 정렬). **게이트: critical 누락 0건이어야 다음 단계.**
3. **기획 보강** (`design-augmenter`) — 검토에서 나온 공백·누락 데이터·상세 스펙을 채움. 출력: `docs/` 갱신 + changelog.
4. **개발 ∥ 디자인** (병렬, worktree) — `gameplay-engineer`는 구현, `art-director`는 비주얼 목업.
5. **검증** (병렬) — `qa-playtester`(플레이테스트) + `code-reviewer-game`(코드) + `size-guardian`(용량). 이슈는 **이슈 단위로 머지**(리뷰어 단위 아님).
6. **피드백 루프** — 이슈 발견 시 개발로 회귀. **최대 3회/태스크**(초과 시 PM 에스컬레이션).
7. **디자인 컨펌 게이트** — 맵/캐릭터/UI는 PNG로 사용자 컨펌 후에만 구현 진행(→ `50-design-confirm.md`).

## 산출물 계약 (artifact contract)
모든 에이전트 보고는 아래 LAW를 따른다(머신 파싱 가능하게):
- **LAW 1** 보고 첫 줄 = `## <에이전트> · <단계> · <태스크ID>`.
- **LAW 2** 발견 항목은 번호 목록, 각 항목 = `[심각도] 제목 — 근거(파일:라인) — 제안`.
- **LAW 3** 심각도 = `CRIT > HIGH > MED > LOW`. CRIT = 출시/빌드/용량 차단.
- **LAW 4** 용량 영향이 있으면 `Δsize: +Nbytes(추정/실측)` 명시.
- **LAW 5** 끝에 `→ NEXT:` 다음 단계/담당 제안 1줄.

## 핸드오프 (PM↔dev↔QA 바통)
- 각 worktree 코디네이션 디렉터리에 `task.md`(무엇을) / `handoff.md`(맥락·미해결질문 먼저) / `status.md`(진행) 유지.
- 핸드오프는 **미해결 질문을 가장 먼저** 적고, 구체적 다음 행동으로 끝낸다.

## 메모리 (세션 간)
- 설계 결정·밸런스 수치·용량 측정 이력은 `.claude/memory/`에 append-only로 기록(`MEMORY.md` 인덱스 1줄 + 개별 파일).
- 태그는 구체적으로(`dash-iframe-0.14s`, `size-after-m3` 등). 모호한 태그 금지.

## 모델 라우팅 (비용 인지)
- 판단·설계·리뷰 = opus. 구현·QA 시나리오 = sonnet. 측정·포맷·반복 = haiku.
- 대량 병렬 fan-out 시 하위 에이전트 모델은 태스크 난이도에 맞춰 격하.

## 적용 범위
- 본 규칙은 게임 코드(`src/`)와 문서(`docs/`, `DESIGN.md`)에 적용. 하네스 자체 설정(`.claude/`) 변경은 사용자 승인 필요.
