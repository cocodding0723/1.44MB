# worktrees.ps1 — 병렬 작업용 git worktree + 코디네이션 파일 생성 (rules/40)
# 사용: powershell -File scripts/worktrees.ps1 -Areas dungeon,combat,audio
# 사전: 이 폴더가 git repo 여야 함(git init). 워크트리는 ..\nd-<area>.
param([Parameter(Mandatory=$true)][string[]]$Areas)

# repo 확인
git rev-parse --is-inside-work-tree 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error "git repo 아님. 먼저 'git init' 필요(rules/40)."; exit 1 }

foreach ($a in $Areas) {
  $wt = "..\nd-$a"
  git worktree add -b "feat/$a" $wt HEAD
  if ($LASTEXITCODE -ne 0) { Write-Warning "worktree 생성 실패: $a (브랜치 중복?)"; continue }
  $coord = Join-Path $wt ".coord"
  New-Item -ItemType Directory -Force $coord | Out-Null
  Set-Content (Join-Path $coord 'task.md')    "# task: $a`n- 범위:`n- 수용 기준:`n- size 예산(rules/10):`n- 의존성:"
  Set-Content (Join-Path $coord 'handoff.md') "# handoff: $a`n## 미해결 질문 (먼저)`n- `n## 맥락`n- `n## 다음 행동`n- "
  Set-Content (Join-Path $coord 'status.md')  "# status: $a`n- [ ] 진행중`n- size: -"
  Write-Host "worktree 생성: $wt  (브랜치 feat/$a, .coord 초기화)"
}
Write-Host "완료. 머지 후 'git worktree remove ..\nd-<area>' 로 정리."
