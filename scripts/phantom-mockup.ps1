# phantom-mockup.ps1 — 신규 적 PHANTOM(위상 침입자) 비주얼 목업 (rules/50, 절차 렌더)
param([string]$Out = "docs\design\enemy-phantom-v1.png")
Add-Type -AssemblyName System.Drawing
$W=1280; $H=720
$bmp=New-Object System.Drawing.Bitmap($W,$H); $g=[System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::FromArgb(255,8,10,18))
function SB($r,$g2,$b,$a){ New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($a,$r,$g2,$b)) }
function PN($r,$g2,$b,$a,$w){ $p=New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb($a,$r,$g2,$b),$w); return $p }
function Diamond($cx,$cy,$rad){ return @((New-Object System.Drawing.PointF($cx,($cy-$rad))),(New-Object System.Drawing.PointF(($cx+$rad),$cy)),(New-Object System.Drawing.PointF($cx,($cy+$rad))),(New-Object System.Drawing.PointF(($cx-$rad),$cy))) }
# 도트 그리드
$dot=SB 0 230 255 26; for($y=60;$y -lt $H;$y+=44){ for($x=60;$x -lt $W;$x+=44){ $g.FillRectangle($dot,$x,$y,2,2) } }
$g.DrawRectangle((PN 0 230 255 70 2),50,50,($W-100),($H-100))
# 색: 스펙트럴 페일바이올렛 코어, 바이올렛 글로우
$cr=210;$cg=199;$cb=255; $gr=170;$gg=140;$gb=255
function Phantom($cx,$cy,$rad,$ghostDx){
  # 글로우 (큰 반투명 다이아)
  $g.FillPolygon((SB $gr $gg $gb 60),(Diamond $cx $cy ($rad*1.8)))
  # 위상 고스트 (오프셋 잔상)
  $g.FillPolygon((SB $cr $cg $cb 90),(Diamond ($cx+$ghostDx) ($cy-$ghostDx*0.4) ($rad*0.95)))
  # 점멸 링
  $g.DrawEllipse((PN $gr $gg $gb 150 2),($cx-$rad-6),($cy-$rad-6),($rad*2+12),($rad*2+12))
  # 코어 다이아
  $g.FillPolygon((SB $cr $cg $cb 240),(Diamond $cx $cy $rad))
  $g.FillPolygon((SB 255 255 255 230),(Diamond $cx $cy ($rad*0.4)))
}
# (1) 평상 위상 — 3개 변주(잔상 오프셋 다름 = 점멸 표현)
Phantom 300 250 26 7
Phantom 470 250 26 -4
Phantom 640 250 26 11
# (2) 텔레그래프 → 순간이동: 현재(흐림) + 점선 + 목적지 고스트 outline
$px=900; $py=440; $tx=1080; $ty=360   # 현재→목적지(플레이어 쪽)
$g.FillPolygon((SB $cr $cg $cb 110),(Diamond $px $py 26))           # 현재(페이드아웃)
$dash=New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(160,$gr,$gg,$gb),2); $dash.DashStyle=[System.Drawing.Drawing2D.DashStyle]::Dash
$g.DrawLine($dash,$px,$py,$tx,$ty)                                   # 점선 경로
$g.DrawPolygon((PN $cr $cg $cb 220 2),[System.Drawing.PointF[]](Diamond $tx $ty 26))         # 목적지 고스트 outline
$g.DrawEllipse((PN $gr $gg $gb 120 1),($tx-34),($ty-34),68,68)
# 플레이어 (흰 다이아 + 시안 글로우) — 텔레그래프 대상
$plx=1180; $ply=300
$g.FillPolygon((SB 0 230 255 70),(Diamond $plx $ply 24))
$g.FillPolygon((SB 255 255 255 255),(Diamond $plx $ply 13))
# 라벨
$fT=New-Object System.Drawing.Font("Consolas",34,[System.Drawing.FontStyle]::Bold)
$fS=New-Object System.Drawing.Font("Consolas",18,[System.Drawing.FontStyle]::Regular)
$sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Center
$g.DrawString("NEW ENEMY  -  PHANTOM (위상 침입자)",$fT,(SB $cr $cg $cb 240),($W/2),70,$sf)
$g.DrawString("이중 잔상 다이아 + 점멸 링 (스펙트럴 페일바이올렛)",$fS,(SB 180 175 220 220),470,320,$sf)
$g.DrawString("주기적 순간이동: 0.5s 텔레그래프(목적지 고스트+점선) → 플레이어 방향 블링크",$fS,(SB 180 175 220 220),($W/2),560,$sf)
$g.DrawString("HP 18 / 느린 드리프트 / 반경 12 / 접촉 1뎀 / DESCENT L7+ · OVERCLOCK t180+",$fS,(SB 150 150 180 200),($W/2),600,$sf)
$g.Dispose()
$dir=Resolve-Path -LiteralPath (Split-Path $Out); $bmp.Save((Join-Path $dir (Split-Path $Out -Leaf)),[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
Write-Output ("saved "+$Out)
