# design-confirm-gate.ps1 — 디자인 이미지 컨펌 리마인더 (rules/50)
# Stop 등에서 호출. 컨펌 대기/수정 항목이 있으면 경고.
$ErrorActionPreference = 'SilentlyContinue'
try { $null = [Console]::In.ReadToEnd() } catch {}
$proj = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$conf = Join-Path $proj 'docs\design\CONFIRMATIONS.md'
if (-not (Test-Path $conf)) { exit 0 }
$txt = Get-Content -Raw $conf
if ($txt -match '⏳' -or $txt -match '🔁') {
  Write-Host "[design] 컨펌 대기/수정 항목 존재(docs/design/CONFIRMATIONS.md). 비주얼 구현 전 PNG 컨펌 필요(rules/50)."
}
exit 0
