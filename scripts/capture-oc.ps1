# capture-oc.ps1 — OVERCLOCK 모드 스모크 캡처 (capture.ps1 기반)
param([string]$OutDir = "docs\design\shots-oc")

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class CapOC {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
  [DllImport("user32.dll")] public static extern bool PostMessageA(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
}
"@

function Get-GameWindow {
  $p = Get-Process -Name game -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
  if (-not $p) { throw "NEON DESCENT window not found" }
  return [IntPtr]$p.MainWindowHandle
}
function Save-Shot([IntPtr]$h, [string]$path) {
  [CapOC+RECT]$r = New-Object CapOC+RECT
  [CapOC]::GetClientRect($h, [ref]$r) | Out-Null
  $w = $r.R - $r.L; $ht = $r.B - $r.T
  if ($w -le 0 -or $ht -le 0) { throw "bad client rect" }
  $bmp = New-Object System.Drawing.Bitmap($w, $ht)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $dc = $g.GetHdc()
  [CapOC]::PrintWindow($h, $dc, 2) | Out-Null
  $g.ReleaseHdc($dc); $g.Dispose()
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
  Write-Output "saved: $path"
}
function Post-Key([IntPtr]$h, [int]$vk, [int]$downMs = 60) {
  [CapOC]::PostMessageA($h, 0x0100, [IntPtr]$vk, [IntPtr]0) | Out-Null
  Start-Sleep -Milliseconds $downMs
  [CapOC]::PostMessageA($h, 0x0101, [IntPtr]$vk, [IntPtr]0) | Out-Null
}
function Post-MouseMove([IntPtr]$h, [int]$x, [int]$y) {
  $l = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
  [CapOC]::PostMessageA($h, 0x0200, [IntPtr]0, $l) | Out-Null
}

$h = Get-GameWindow
New-Item -ItemType Directory -Force $OutDir | Out-Null

Start-Sleep -Milliseconds 700
Save-Shot $h (Join-Path $OutDir "01-title-descent.png")   # 기본 DESCENT

Post-Key $h 0x09                                          # TAB → OVERCLOCK
Start-Sleep -Milliseconds 400
Save-Shot $h (Join-Path $OutDir "02-title-overclock.png")

Post-Key $h 0x20                                          # SPACE → 시작
Start-Sleep -Milliseconds 1500
Post-MouseMove $h 700 360
Save-Shot $h (Join-Path $OutDir "03-arena-start.png")     # 아레나 진입 (HUD: XP/LV/타이머)

Start-Sleep -Milliseconds 4000                            # 호드 누적 + 자동사격
Post-MouseMove $h 640 360
Save-Shot $h (Join-Path $OutDir "04-horde.png")

Start-Sleep -Milliseconds 6000                            # 더 누적 → 레벨업 드래프트 가능
Save-Shot $h (Join-Path $OutDir "05-later.png")
Write-Output "done"
