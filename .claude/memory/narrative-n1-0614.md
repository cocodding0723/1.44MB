---
name: narrative-n1-0614
description: 2026-06-14 서사(세피리아풍) N1 골격 — 본편 DESCENT에 영문 기계 교신체 구현. OVERCLOCK은 red-team 게이트로 보류
metadata:
  type: project
---

# 서사 N1 골격 (2026-06-14)

사용자 요청: VS/세피리아풍 모드(OVERCLOCK SDD=docs/07) + 세피리아풍 스토리(내러티브=docs/08).
**design-reviewer red-team 결과 OVERCLOCK은 착수 게이트 통과 못함**(재활용 ~40%, 렌더 배칭 비현실·결정론 시드 부재·보스 그리드 결합·draw3 모듈종속·+15~25KB) → **보류**. 서사는 본편 DESCENT에 얹으면 저위험이라 **서사 먼저** 결정(사용자 승인).

## 확정 (사용자)
- 언어 = **영문 대문자 기계 교신체** (3×5 폰트 영문전용, 한글 미채택 — 수천 글리프 필요). 폰트에 `,!?'` +4글리프 추가.
- **메타 세이브 도입** 확정(누적 부패도·해금·코덱스) — 단 N2로 분리(이번엔 in-run 서사만).
- 서사 주 무대 = DESCENT. OVERCLOCK은 프레이밍 공유.

## N1 구현 (in-run 서사, 본편 DESCENT)
- 폰트 40→44글리프(`,!?'`), `draw_text_multi`(개행 분할 + 타이핑 cutoff) 신규.
- 교신 테이블: `g_xmitLayer[6]`(narr_zone 깊이 매핑), `g_xmitBoss[3]`(CORE/WARDEN/NEXUS 인격 대사), `g_xmitDeath[4]`(에피타프).
- 트리거: new_run(레이어1)·descend(레이어 진입)·보스 스폰·사망. 전역 `g_xmitMsg/g_xmitT/g_deathMsg`, `set_xmit()`.
- 렌더: render_hud 비차단 오버레이(좌하단 ECHO 시질바+다줄 타이핑, 0.3s 페이드인/5.5s 페이드아웃, 30char/s). 게임오버 에피타프. 타이머는 PLAY에서 dt 누적(표시 전용, sim 결정론 무영향 → red-team sim정지 매트릭스 자연 해결).
- 세계관: THE MAINFRAME 부패(THE ROT), 주인공 REVENANT(삭제된 의식 파편), ECHO(신뢰불가 안내 AI), 팰릿 5티어=하강 챕터(냉각/연산/메모리/전력/커널).
- 빌드 81,408B(캡 5.5%). 인게임 캡처 확인(docs/design/shots-narr).

## N1 핵심 교훈 (red-team)
- **`hurt_player` self-i-frame 가드**(진입부 g_pIfr 체크)는 다중피격 오판 방지에 load-bearing.
- 본편 시드가 QPC라 비결정론 — OVERCLOCK 결정론 테스트엔 고정시드 빌드훅 선결 필요.
- 즉시모드 GL은 적별 5~9 glBegin라 256적 배칭 = 렌더러 재작성(재활용 아님).
- no-CRT 세이브: memset 자작(OK), shell32 미링크 → LOCALAPPDATA는 GetEnvironmentVariable.

## N2 메타 세이브 (2026-06-14, 검증 완료)
- 세이브: `%LOCALAPPDATA%\neondescent.dat` 32B(magic NDRN·ver·bestScore·bestLayer·corruption·codex·bestTime·FNV체크섬). **shell32 미링크 → GetEnvironmentVariableA로 경로**, CreateFileA/WriteFile, **원자적 쓰기(temp→MoveFileExA REPLACE|WRITE_THROUGH)**, 로드 시 magic+ver+크기+체크섬 4중검증→불일치 시 기본값. memset 자작 확인.
- 코덱스 8파편(`g_codexTxt`), `unlock_codex(bit)`=해금+오버레이+저장. 마일스톤: descend 깊이(L2/4/6/9), boss_die(처치/WARDEN/NEXUS), new_run 부패도(≥3/≥8). 부패도별 ECHO 변주("YOU ARE AWAKE. AGAIN. HOW MANY TIMES NOW").
- ST_CODEX 뷰어(타이틀 C), 타이틀에 "CODEX n/8" 표시. 사망 시 corruption++ + save_write.
- **라운드트립 검증**: 임시 K디버그로 unlock→32B 파일(magic/ver/corruption/codex/crc 확인)→재실행 save_load→코덱스에 FRAG03 영속 표시 확인. 디버그 되돌림. 84,992B(캡 5.8%).
- **교훈**: 디버그 키 추가 후 반드시 깨끗한 재빌드 확인(에러 스크립트 안 빌드는 미완료될 수 있음 — K가 안 먹던 원인). 세이브 파일 Remove-Item은 샌드박스 차단 → 덮어쓰기로 테스트.

## 미해결
- N3 페르소나/엔딩(REVENANT 외 DAEMON/SENTINEL/GHOST, 다중 엔딩). 무기 진화.
- 보스/사망 교신은 코드 경로 동일(레이어 교신 검증됨)이나 인게임 실측 미완(보스 도달 어려움).
- 서사 비주얼(시질·교신 UI) rules/50 ⏳.
