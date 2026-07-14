# size-budget-check.ps1 — 1.44MB 용량 예산 게이트 (rules/10)
# PostToolUse(Bash/PowerShell)·Stop 에서 호출. build\game.exe 측정·판정·이력기록.
# FAIL(>cap) 시 exit 2로 차단, 그 외 경고만.
$ErrorActionPreference = 'SilentlyContinue'
try { $null = [Console]::In.ReadToEnd() } catch {}

$cap=1474560; $target=65536; $warn=49152; $hardWarn=1179648
$proj = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$exe = Join-Path $proj 'build\game.exe'
if (-not (Test-Path $exe)) { exit 0 }   # 아직 빌드 없음

$size = (Get-Item $exe).Length
$pctCap = [math]::Round($size*100.0/$cap,2)
$pctTgt = [math]::Round($size*100.0/$target,2)
if     ($size -gt $cap)      { $v='FAIL' }
elseif ($size -ge $hardWarn) { $v='RED' }
elseif ($size -ge $target)   { $v='ORANGE' }
elseif ($size -ge $warn)     { $v='YELLOW' }
else                         { $v='GREEN' }

$hist = Join-Path $proj '.claude\memory\size-history.md'
# dedup: 직전 표 행과 크기가 같으면 기록 생략(매 툴호출 스팸 방지). 빌드로 크기가 바뀔 때만 1행 추가.
$dup = $false
try {
  $lastRow = Get-Content -Path $hist -ErrorAction SilentlyContinue | Where-Object { $_ -match '^\|' } | Select-Object -Last 1
  if ($lastRow -like "*| $size |*") { $dup = $true }
} catch {}
if (-not $dup) {
  try { Add-Content -Path $hist -Value ("| {0} | auto | {1} | {2}% | {3}% | {4} |" -f (Get-Date -Format 'yyyy-MM-dd HH:mm'), $size, $pctCap, $pctTgt, $v) } catch {}
}

if ($v -eq 'FAIL') {
  Write-Error "[SIZE GATE] FAIL: game.exe=$size bytes > 1,474,560 cap. 빌드 거부 — 감축 필요(rules/10)."
  exit 2
}
Write-Host "[size] $size bytes ($pctCap% cap / $pctTgt% target) = $v"
exit 0
