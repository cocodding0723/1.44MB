# mockup.ps1 — 네온 벡터 디자인 목업 PNG 생성 (rules/50, 외부 의존 0)
# art-director 가 호출하는 베이스. System.Drawing 으로 도형+글로우+팰릿 렌더.
param(
  [string]$Out = "docs\design\mockup-v1.png",
  [int]$W = 1280,
  [int]$H = 720,
  [int[]]$Bg = @(8,10,16),
  [int[]]$Accent = @(0,230,255)
)
Add-Type -AssemblyName System.Drawing
$bmp = New-Object System.Drawing.Bitmap $W, $H
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = 'AntiAlias'
$g.Clear([System.Drawing.Color]::FromArgb($Bg[0], $Bg[1], $Bg[2]))

function Neon-Rect($g,$x,$y,$w,$h,$c) {
  for ($i=5; $i -ge 1; $i--) {
    $a = [int](36/$i)
    $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb($a,$c[0],$c[1],$c[2])), ($i*3)
    $g.DrawRectangle($pen, $x, $y, $w, $h); $pen.Dispose()
  }
  $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(255,$c[0],$c[1],$c[2])), 2
  $g.DrawRectangle($pen, $x, $y, $w, $h); $pen.Dispose()
}
function Neon-Poly($g,[float[]]$pts,$c) {
  $points = @(); for($i=0;$i -lt $pts.Length;$i+=2){ $points += New-Object System.Drawing.PointF($pts[$i],$pts[$i+1]) }
  for ($i=5; $i -ge 1; $i--) {
    $a=[int](36/$i)
    $pen=New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb($a,$c[0],$c[1],$c[2])),($i*3)
    $g.DrawPolygon($pen,$points); $pen.Dispose()
  }
  $pen=New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(255,$c[0],$c[1],$c[2])),2
  $g.DrawPolygon($pen,$points); $pen.Dispose()
}

# 예시 구성: 방 프레임 + 플레이어 다이아 (art-director가 요소별로 확장)
Neon-Rect $g 100 90 ($W-200) ($H-180) $Accent
$cx=$W/2; $cy=$H/2
Neon-Poly $g @($cx,($cy-26),($cx+22),$cy,$cx,($cy+26),($cx-22),$cy) @(255,255,255)

$dir = Split-Path -Parent $Out
if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Host "mockup saved: $Out ($W x $H)"
