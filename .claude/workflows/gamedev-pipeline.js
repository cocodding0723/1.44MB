export const meta = {
  name: 'gamedev-pipeline',
  description: 'NEON DESCENT 개발팀 파이프라인: PM→기획검토→기획보강→[개발∥디자인]→[QA+코드리뷰+용량]→피드백 루프. 1.44MB 용량 게이트 내장.',
  phases: [
    { title: 'PM 계획' },
    { title: '기획 검토' },
    { title: '기획 보강' },
    { title: '개발 ∥ 디자인' },
    { title: '검증(QA·리뷰·용량)' },
    { title: '피드백 루프' },
  ],
}

// args: { task, areas:[], maxFeedback }
const task = (args && args.task) || 'M1 엔진 골격';
const areas = (args && args.areas) || ['engine'];
const maxFeedback = (args && args.maxFeedback) || 3;

const REPORT = {
  type: 'object',
  properties: {
    summary: { type: 'string' },
    issues: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          severity: { type: 'string', description: 'CRIT|HIGH|MED|LOW' },
          title: { type: 'string' },
          detail: { type: 'string' },
        },
        required: ['severity', 'title'],
      },
    },
    sizeBytes: { type: 'number', description: '측정된 exe 바이트(있으면)' },
  },
  required: ['summary', 'issues'],
}
const isBlocking = (i) => /CRIT|HIGH/i.test(i.severity || '')

// 1) PM 계획
phase('PM 계획')
const plan = await agent(
  `너는 pm-orchestrator. SoT(DESIGN.md, docs/01~05, .claude/rules/*)를 읽고 태스크 "${task}"를 분해해 계획을 세워라.\n각 태스크 = {범위, 수용기준, size예산(rules/10 §3), 의존성, 담당 에이전트}. 병렬 가능 영역: ${areas.join(', ')}. 용량 사수 > 게임성 우선순위 적용.`,
  { agentType: 'pm-orchestrator', phase: 'PM 계획' }
)

// 2) 기획 검토 (red-team)
phase('기획 검토')
const review = await agent(
  `너는 design-reviewer. "${task}" 관련 기획(DESIGN.md/docs)을 red-team하여 문제·모순·누락을 보고. CRIT 누락이 있으면 "개발 착수 불가" 명시. rules/00 LAW 준수.`,
  { agentType: 'design-reviewer', phase: '기획 검토', schema: REPORT }
)

// 3) 기획 보강 (필요 시)
phase('기획 보강')
if (review.issues && review.issues.some(isBlocking)) {
  await agent(
    `너는 design-augmenter. 아래 검토 공백을 구체 수치/수식/엣지케이스로 채워 docs/DESIGN을 보강하라(스코프 확장 금지, 용량·no-CRT 충돌 금지):\n${JSON.stringify(review.issues)}`,
    { agentType: 'design-augmenter', phase: '기획 보강' }
  )
} else {
  log('기획 검토 통과 — 보강 생략')
}

// 4) 개발 ∥ 디자인 (병렬)
phase('개발 ∥ 디자인')
const built = await parallel([
  () => agent(
    `너는 gameplay-engineer. 태스크 "${task}"를 no-CRT C로 구현. rules/10(용량)·20(no-CRT)·30(verify-loop) 준수. build.bat로 size 실측하고 Δsize·용량판정 보고. 컨펌 안 된 비주얼 구현 금지(rules/50).`,
    { agentType: 'gameplay-engineer', phase: '개발 ∥ 디자인', schema: REPORT }
  ),
  () => agent(
    `너는 art-director. "${task}" 관련 비주얼 요소가 있으면 scripts/mockup.ps1로 PNG 목업(docs/design/) 생성 후 CONFIRMATIONS.md 갱신·컨펌 요청. 없으면 해당 없음 보고. rules/50.`,
    { agentType: 'art-director', phase: '개발 ∥ 디자인' }
  ),
])
const devResult = built[0]
const artResult = built[1]

// 5) 검증 + 피드백 루프
let round = 0
let lastVerify = null
while (round <= maxFeedback) {
  phase('검증(QA·리뷰·용량)')
  const verify = await parallel([
    () => agent(
      `너는 qa-playtester. docs/05_검사기준서의 TC 기반으로 "${task}" 빌드를 검증. 버그는 재현(시드+단계) 포함 보고. rules/00 LAW.`,
      { agentType: 'qa-playtester', phase: '검증(QA·리뷰·용량)', schema: REPORT }
    ),
    () => agent(
      `너는 code-reviewer-game. "${task}" 변경 코드를 검토(용량[최우선]/no-CRT/정확성/결정론). Δsize·용량판정 포함. rules/00 LAW.`,
      { agentType: 'code-reviewer-game', phase: '검증(QA·리뷰·용량)', schema: REPORT }
    ),
    () => agent(
      `너는 size-guardian. build.bat로 exe 측정·판정(1.44MB cap / 64KB target)·이력 기록. FAIL이면 차단 신호.`,
      { agentType: 'size-guardian', phase: '검증(QA·리뷰·용량)', schema: REPORT }
    ),
  ])
  lastVerify = verify
  const blocking = verify.filter(Boolean).flatMap(r => (r.issues || []).filter(isBlocking))
  if (blocking.length === 0) {
    log(`검증 통과 (round ${round + 1}) — 차단 이슈 0`)
    break
  }
  if (round === maxFeedback) {
    log(`⚠ 피드백 ${maxFeedback}회 소진, 차단 이슈 ${blocking.length}건 잔존 → PM 에스컬레이션 필요`)
    break
  }
  round++
  phase('피드백 루프')
  log(`차단 이슈 ${blocking.length}건 → gameplay-engineer 회귀 (round ${round}/${maxFeedback})`)
  await agent(
    `너는 gameplay-engineer. 아래 검증 이슈를 수정하라(용량·no-CRT 준수, 재빌드·size 실측):\n${JSON.stringify(blocking)}`,
    { agentType: 'gameplay-engineer', phase: '피드백 루프', schema: REPORT }
  )
}

return {
  task,
  plan,
  review,
  dev: devResult,
  art: artResult,
  verify: lastVerify,
  feedbackRoundsUsed: round,
}
