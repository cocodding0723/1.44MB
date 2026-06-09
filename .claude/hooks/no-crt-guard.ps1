# no-crt-guard.ps1 — 금지 CRT 함수 인입 경고 (rules/20)
# PostToolUse(Write/Edit) 에서 호출. src\*.c|h 편집 시 금지 토큰 스캔(경고만).
$ErrorActionPreference = 'SilentlyContinue'
$raw = ''
try { $raw = [Console]::In.ReadToEnd() } catch {}
$fp = $null
try { $j = $raw | ConvertFrom-Json; $fp = $j.tool_input.file_path } catch {}
if (-not $fp) { exit 0 }
if ($fp -notmatch '\\src\\.*\.(c|h)$') { exit 0 }
if (-not (Test-Path $fp)) { exit 0 }

$txt = Get-Content -Raw $fp
$patterns = @('\bmalloc\b','\bcalloc\b','\bfree\b','\bprintf\b','\bsprintf\b','\bsscanf\b',
              '\bsinf?\b','\bcosf?\b','\bsqrtf?\b','\bpowf?\b','\brand\b','\bsrand\b','\bfopen\b')
$hits = @()
foreach ($p in $patterns) { if ($txt -match $p) { $hits += ($p -replace '\\b','' -replace '\?','') } }
if ($hits.Count) {
  Write-Host "[no-crt] 주의: 금지 CRT 후보 발견 ($([string]::Join(', ', ($hits | Select-Object -Unique)))) in $fp — rules/20 확인. 자작 구현/인트린식으로 대체할 것."
}
exit 0
