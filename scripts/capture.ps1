# capture.ps1 — NEON DESCENT 창을 포커스 뺏지 않고 캡처 + 키 이벤트 주입 (rules/50 컨펌 자료)
# PrintWindow(PW_RENDERFULLCONTENT) + PostMessage(WM_KEYDOWN) — 사용자 데스크톱 무간섭
param([string]$OutDir = "docs\design")

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Cap {
  [DllImport("user32.dll")] public static extern IntPtr FindWindowA(string cls, string title);
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
  [Cap+RECT]$r = New-Object Cap+RECT
  [Cap]::GetClientRect($h, [ref]$r) | Out-Null
  $w = $r.R - $r.L; $ht = $r.B - $r.T
  if ($w -le 0 -or $ht -le 0) { throw "bad client rect" }
  $bmp = New-Object System.Drawing.Bitmap($w, $ht)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $dc = $g.GetHdc()
  [Cap]::PrintWindow($h, $dc, 2) | Out-Null  # 2 = PW_RENDERFULLCONTENT (GL 내용 포함)
  $g.ReleaseHdc($dc); $g.Dispose()
  $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  Write-Output "saved: $path"
}

function Post-Key([IntPtr]$h, [int]$vk, [int]$downMs = 60) {
  [Cap]::PostMessageA($h, 0x0100, [IntPtr]$vk, [IntPtr]0) | Out-Null   # WM_KEYDOWN
  Start-Sleep -Milliseconds $downMs
  [Cap]::PostMessageA($h, 0x0101, [IntPtr]$vk, [IntPtr]0) | Out-Null   # WM_KEYUP
}
function Post-MouseMove([IntPtr]$h, [int]$x, [int]$y) {
  $l = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
  [Cap]::PostMessageA($h, 0x0200, [IntPtr]0, $l) | Out-Null            # WM_MOUSEMOVE
}
function Post-LMB([IntPtr]$h, [int]$x, [int]$y, [int]$holdMs = 100) {
  $l = [IntPtr](($y -shl 16) -bor ($x -band 0xFFFF))
  [Cap]::PostMessageA($h, 0x0201, [IntPtr]1, $l) | Out-Null            # WM_LBUTTONDOWN
  Start-Sleep -Milliseconds $holdMs
  [Cap]::PostMessageA($h, 0x0202, [IntPtr]0, $l) | Out-Null            # WM_LBUTTONUP
}

$h = Get-GameWindow
New-Item -ItemType Directory -Force $OutDir | Out-Null

# 1. 타이틀
Start-Sleep -Milliseconds 600
Save-Shot $h (Join-Path $OutDir "shot-title.png")

# 2. 게임 시작 (Space) → 플레이 화면
Post-Key $h 0x20
Start-Sleep -Milliseconds 900
Post-MouseMove $h 900 300
Save-Shot $h (Join-Path $OutDir "shot-play.png")

# 3. 이동 + 사격 (W 홀드 + LMB) → 전투 비주얼
[Cap]::PostMessageA($h, 0x0100, [IntPtr]0x57, [IntPtr]0) | Out-Null  # W down
Post-LMB $h 900 300 700
[Cap]::PostMessageA($h, 0x0101, [IntPtr]0x57, [IntPtr]0) | Out-Null  # W up
Save-Shot $h (Join-Path $OutDir "shot-combat.png")

# 4. 대시 잔상
Post-Key $h 0x20
Start-Sleep -Milliseconds 120
Save-Shot $h (Join-Path $OutDir "shot-dash.png")

# 5. 일시정지 (ESC)
Post-Key $h 0x1B
Start-Sleep -Milliseconds 400
Save-Shot $h (Join-Path $OutDir "shot-pause.png")
Post-Key $h 0x1B
