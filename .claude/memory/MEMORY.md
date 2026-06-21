# 프로젝트 메모리 인덱스 (NEON DESCENT)

세션 간 유지되는 핵심 사실의 인덱스. 각 항목 1줄, 상세는 개별 파일.

- [design-frozen](design-frozen.md) — GDD v1.0 FROZEN(DESIGN.md), 실시간 액션 로그라이크 + 마우스 트윈스틱 + 기계 테마 확정
- [size-history](size-history.md) — exe 용량 측정 이력(1.44MB 예산 추적)
- [harness](harness.md) — 멀티에이전트 하네스 구성(에이전트 8·규칙 6·훅·워크플로)
- [m4-m6-ext-session](m4-m6-ext-session.md) — M4~M6+확장(타격감·스킬·장애물) 완료, 결정론 보호/슬로모/EMP 설계 결정, capture.ps1 무간섭 캡처 도구
- [verify-session-0613](verify-session-0613.md) — ultracode 6차원 적대적 리뷰(CRIT/HIGH 0) + 9건 수정(카메라 dt-스케일·코스메틱 RNG 분리·재생 비스택·어픽스 중복방지·자력 오라·게임오버 BEST). 여전히 미커밋
- [expansion-AB-0613](expansion-AB-0613.md) — FROZEN 초과 승인 확장: A(다층 스프라이트·보스 페이즈 시각화·셰이더리스 포스트FX, ✅승인) + B(적 LANCER/WEAVER·보스 NEXUS·모듈 6종) + v1.3a QA 밸런스. origin/master 푸시됨
- [narrative-n1-0614](narrative-n1-0614.md) — 세피리아풍 서사 N1: 본편 DESCENT에 영문 기계 교신체(ECHO/REVENANT, 레이어/보스/사망 교신). OVERCLOCK 모드(docs/07 SDD)는 red-team 게이트로 보류. 81,408B
- [code-structure-0622](code-structure-0622.md) — src/ 도메인 .inc 분할(단일 TU, game.c만 컴파일) + 적 스탯 테이블(g_eneStat). 적/무기/모듈 확장 시 어디 고치는지
- [overclock-o1-0618](overclock-o1-0618.md) — 2026-06-18 세션: OVERCLOCK 모드 O1+O2(g_mode 분기·아레나·자동사격·XP/레벨·보스웨이브·무기 8종) + BGM(§16.4) + 상점(§13, rules/50 승인). boss_spawn 추출(§14.3)·공용 HUD헬퍼 리팩터. 본편 동결 보존. 헤드리스 환경 GL 영상캡처 불가 교훈. 105,472B(캡 7.2%)

> 규칙: 설계 결정·밸런스 수치·용량 이력만 기록. 코드 구조·git 이력 등 repo가 이미 가진 정보는 기록 금지. 태그는 구체적으로.
