# qa-walk.ps1 — 입력 주입 플레이테스트: 방 탐험·전투·스킬 발동 캡처 (포커스 무간섭)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Cap2 {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
  [DllImport("user32.dll")] public static extern bool PostMessageA(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@
$p = Get-Process -Name game -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { throw "game not running" }
$h = [IntPtr]$p.MainWindowHandle
$out = "docs\design\qa"
New-Item -ItemType Directory -Force $out | Out-Null

function Shot([string]$name) {
  [Cap2+RECT]$r = New-Object Cap2+RECT
  [Cap2]::GetClientRect($h, [ref]$r) | Out-Null
  $bmp = New-Object System.Drawing.Bitmap(($r.R - $r.L), ($r.B - $r.T))
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $dc = $g.GetHdc()
  [Cap2]::PrintWindow($h, $dc, 2) | Out-Null
  $g.ReleaseHdc($dc); $g.Dispose()
  $bmp.Save((Join-Path $out $name), [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  Write-Output "shot: $name"
}
function KeyDown([int]$vk) { [Cap2]::PostMessageA($h, 0x0100, [IntPtr]$vk, [IntPtr]0) | Out-Null }
function KeyUp([int]$vk)   { [Cap2]::PostMessageA($h, 0x0101, [IntPtr]$vk, [IntPtr]0) | Out-Null }
function Tap([int]$vk)     { KeyDown $vk; Start-Sleep -Milliseconds 60; KeyUp $vk }
function MouseAt([int]$x, [int]$y) {
  $l = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
  [Cap2]::PostMessageA($h, 0x0200, [IntPtr]0, $l) | Out-Null
}
function FireHold([int]$x, [int]$y, [int]$ms) {
  $l = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
  [Cap2]::PostMessageA($h, 0x0201, [IntPtr]1, $l) | Out-Null
  Start-Sleep -Milliseconds $ms
  [Cap2]::PostMessageA($h, 0x0202, [IntPtr]0, $l) | Out-Null
}
function Blink([int]$x, [int]$y) {
  $l = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
  MouseAt $x $y
  [Cap2]::PostMessageA($h, 0x0204, [IntPtr]2, $l) | Out-Null   # WM_RBUTTONDOWN
}
function Walk([int]$vk, [int]$ms) { KeyDown $vk; Start-Sleep -Milliseconds $ms; KeyUp $vk }

# 시작 (타이틀이면 Space로 진입; 이미 플레이 중이면 대시 1회 발생 — 무해)
Tap 0x20
Start-Sleep -Milliseconds 800

# 4방향으로 길게 걸어 다른 방 탐색 (각 방향 후 캡처)
$dirs = @(@(0x44,"e"), @(0x53,"s"), @(0x41,"w"), @(0x57,"n"))  # D S A W
$i = 0
foreach ($d in $dirs) {
  $i++
  Walk $d[0] 2600
  MouseAt 800 320
  FireHold 800 320 500
  Shot ("walk{0}-{1}.png" -f $i, $d[1])
}
# 스킬: EMP (Q) + 캡처
Tap 0x51
Start-Sleep -Milliseconds 150
Shot "emp.png"
# 블링크 (RMB) + 캡처
Blink 1000 200
Start-Sleep -Milliseconds 200
Shot "blink.png"
# 추가 탐험: 동쪽으로 더
Walk 0x44 3000
Shot "walk-far-e.png"
Walk 0x57 3000
Shot "walk-far-n.png"
