# 40 — Git Worktree 병렬 작업

병렬 에이전트/팀이 파일 충돌 없이 동시에 작업하도록 **branch-backed worktree 1개 = 워커 1명**.

## 토폴로지
- 게임은 자체 git repo(`1.44/`). 워크트리는 형제 디렉터리에 생성.
- 명명: 브랜치 `feat/<area>` `fix/<area>` `design/<area>`, 워크트리 `../nd-<worker>`.
```
git worktree add -b feat/dungeon ../nd-dungeon HEAD
git worktree add -b feat/combat  ../nd-combat  HEAD
```

## 코디네이션 파일 (각 워크트리 `.coord/`)
- `task.md` — 무엇을(범위·수용 기준·size 예산)
- `handoff.md` — 맥락 + **미해결 질문 먼저** + 다음 행동
- `status.md` — 진행/블로커
> `.claude/workflows/` 또는 `scripts/worktrees.ps1`가 plan에서 자동 생성.

## 병렬 분리 원칙
- **서로의 출력에 의존하는 태스크는 병렬화하지 말 것**(ECC 원칙). 의존 시 순차.
- 영역 분리 예: 던전생성 ∥ 사운드 ∥ UI (독립). 전투↔적AI는 인터페이스 합의 후 분리.
- 공유 파일(`src/game.c` 단일 파일 구조면) 충돌 위험 → 초기엔 **모듈 분할**(`src/*.c`)로 영역 격리 권장.

## 머지 규율
- 워커 완료 → `status.md` done → **코드리뷰 + size-check 통과** 후 머지.
- 머지 후 워크트리 제거(`git worktree remove`), 브랜치 정리.
- 충돌은 이슈 단위로 PM이 조정. `--force` 푸시 금지.

## 커밋
- 사용자 명시 요청 시에만 커밋/푸시. 메시지에 `Δsize` 포함 권장(예: `feat(dungeon): MST 연결 (Δsize +1.2KB, 18KB/64KB)`).
- ultracode/Workflow의 `isolation: 'worktree'`는 파일 변경 병렬 시에만(비용 큼).
