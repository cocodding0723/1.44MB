# qa-death.ps1 — 사망 판정 검증: 전투방 진입 후 정지 → GAMEOVER 화면 확인
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Cap3 {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
  [DllImport("user32.dll")] public static extern bool PostMessageA(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@
$p = Get-Process -Name game -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { throw "game not running" }
$h = [IntPtr]$p.MainWindowHandle
function Shot([string]$name) {
  [Cap3+RECT]$r = New-Object Cap3+RECT
  [Cap3]::GetClientRect($h, [ref]$r) | Out-Null
  $bmp = New-Object System.Drawing.Bitmap(($r.R - $r.L), ($r.B - $r.T))
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $dc = $g.GetHdc()
  [Cap3]::PrintWindow($h, $dc, 2) | Out-Null
  $g.ReleaseHdc($dc); $g.Dispose()
  $bmp.Save("docs\design\qa\$name", [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  Write-Output "shot: $name"
}
function Tap([int]$vk) { [Cap3]::PostMessageA($h, 0x0100, [IntPtr]$vk, [IntPtr]0) | Out-Null; Start-Sleep -Milliseconds 60; [Cap3]::PostMessageA($h, 0x0101, [IntPtr]$vk, [IntPtr]0) | Out-Null }
function Walk([int]$vk, [int]$ms) { [Cap3]::PostMessageA($h, 0x0100, [IntPtr]$vk, [IntPtr]0) | Out-Null; Start-Sleep -Milliseconds $ms; [Cap3]::PostMessageA($h, 0x0101, [IntPtr]$vk, [IntPtr]0) | Out-Null }

Tap 0x20            # 타이틀 → 시작
Start-Sleep -Milliseconds 800
# 사방으로 이동하며 전투방 탐색 (잠기면 confine으로 어차피 갇힘)
Walk 0x44 4500      # D 동쪽
Walk 0x57 4000      # W 북쪽
Walk 0x53 7000      # S 남쪽
Shot "death-1-explore.png"
# 정지 — 적 접촉으로 피격 누적 대기 (1뎀/초 상한, 6HP)
Start-Sleep -Seconds 14
Shot "death-2-wait.png"
Start-Sleep -Seconds 14
Shot "death-3-final.png"
