# shop-mockup.ps1 — DESCENT 상점방 비주얼 목업 (rules/50 컨펌용, 절차 렌더). System.Drawing.
# DESIGN §13 스트레치: 비보스 레이어 35% 출현, 판매 3종(모듈 60 / 수리 45 / 리롤 30 BITS), E로 구매.
param([string]$Out = "docs\design\shop-v1.png")
Add-Type -AssemblyName System.Drawing
$W=1280; $H=720
$bmp=New-Object System.Drawing.Bitmap($W,$H)
$g=[System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint=[System.Drawing.Text.TextRenderingHint]::AntiAlias
# 냉각층 팰릿 (시안 액센트, 어두운 청흑 배경)
$bg=[System.Drawing.Color]::FromArgb(255,8,12,18); $g.Clear($bg)
$cyan=[System.Drawing.Color]::FromArgb(255,0,230,255)
$cyanDim=[System.Drawing.Color]::FromArgb(90,0,230,255)
$amber=[System.Drawing.Color]::FromArgb(255,255,200,70)
$pink=[System.Drawing.Color]::FromArgb(255,255,80,150)
function Pen($c,$w){ New-Object System.Drawing.Pen($c,$w) }
function SBrush($c){ New-Object System.Drawing.SolidBrush($c) }
# 바닥 도트 그리드 (기판 느낌)
$dot=SBrush ([System.Drawing.Color]::FromArgb(36,0,230,255))
for($y=70;$y -lt ($H-70);$y+=48){ for($x=70;$x -lt ($W-70);$x+=48){ $g.FillRectangle($dot,$x,$y,2,2) } }
# 방 외곽 회로 라인
$wall=Pen $cyan 2; $g.DrawRectangle($wall,60,60,($W-120),($H-120))
$wallDim=Pen $cyanDim 1; $g.DrawRectangle($wallDim,52,52,($W-104),($H-104))
# 타이틀
$fTitle=New-Object System.Drawing.Font("Consolas",46,[System.Drawing.FontStyle]::Bold)
$fLbl=New-Object System.Drawing.Font("Consolas",22,[System.Drawing.FontStyle]::Bold)
$fPrice=New-Object System.Drawing.Font("Consolas",20,[System.Drawing.FontStyle]::Regular)
$fHint=New-Object System.Drawing.Font("Consolas",18,[System.Drawing.FontStyle]::Regular)
$sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Center
$g.DrawString("RECLAMATION TERMINAL",$fTitle,(SBrush $cyan),($W/2),90,$sf)
$g.DrawString("SPEND BITS - WALK OVER + E",$fHint,(SBrush $cyanDim),($W/2),155,$sf)
# 제단 3개 (네온 다이아 + 라벨 + 가격)
$items=@(
  @{x=320; name="MODULE";  sub="INSTALL 1 OF 3"; price="60"; col=$cyan},
  @{x=640; name="REPAIR";  sub="+1 HEART (2 HP)"; price="45"; col=$pink},
  @{x=960; name="REROLL";  sub="RANDOM MODULE";   price="30"; col=$amber}
)
foreach($it in $items){
  $cx=$it.x; $cy=380; $c=$it.col
  # 제단 받침 (사각 글로우)
  $g.FillRectangle((SBrush ([System.Drawing.Color]::FromArgb(40,$c.R,$c.G,$c.B))),($cx-95),($cy-95),190,190)
  $g.DrawRectangle((Pen $c 2),($cx-95),($cy-95),190,190)
  # 부유 다이아 (2겹)
  $pts=@( (New-Object System.Drawing.PointF($cx,($cy-46))), (New-Object System.Drawing.PointF(($cx+46),$cy)), (New-Object System.Drawing.PointF($cx,($cy+46))), (New-Object System.Drawing.PointF(($cx-46),$cy)) )
  $g.FillPolygon((SBrush ([System.Drawing.Color]::FromArgb(120,$c.R,$c.G,$c.B))),$pts)
  $pts2=@( (New-Object System.Drawing.PointF($cx,($cy-26))), (New-Object System.Drawing.PointF(($cx+26),$cy)), (New-Object System.Drawing.PointF($cx,($cy+26))), (New-Object System.Drawing.PointF(($cx-26),$cy)) )
  $g.FillPolygon((SBrush ([System.Drawing.Color]::FromArgb(235,255,255,255))),$pts2)
  # 라벨/서브/가격
  $g.DrawString($it.name,$fLbl,(SBrush $c),$cx,($cy+110),$sf)
  $g.DrawString($it.sub,$fHint,(SBrush $cyanDim),$cx,($cy+145),$sf)
  # BITS 가격 (앰버 다이아 + 숫자)
  $bx=$cx-30; $by=$cy+185
  $bpts=@( (New-Object System.Drawing.PointF($bx,($by-8))), (New-Object System.Drawing.PointF(($bx+8),$by)), (New-Object System.Drawing.PointF($bx,($by+8))), (New-Object System.Drawing.PointF(($bx-8),$by)) )
  $g.FillPolygon((SBrush $amber),$bpts)
  $g.DrawString($it.price+" BITS",$fPrice,(SBrush $amber),($cx+14),($by-14))
}
# 플레이어 (흰 코어 다이아 + 시안 글로우) — MODULE 제단 근처
$px=320; $py=560
$g.FillPolygon((SBrush ([System.Drawing.Color]::FromArgb(70,0,230,255))),@( (New-Object System.Drawing.PointF($px,($py-26))),(New-Object System.Drawing.PointF(($px+26),$py)),(New-Object System.Drawing.PointF($px,($py+26))),(New-Object System.Drawing.PointF(($px-26),$py)) ))
$g.FillPolygon((SBrush ([System.Drawing.Color]::White)),@( (New-Object System.Drawing.PointF($px,($py-14))),(New-Object System.Drawing.PointF(($px+14),$py)),(New-Object System.Drawing.PointF($px,($py+14))),(New-Object System.Drawing.PointF(($px-14),$py)) ))
$g.DrawString("E",$fLbl,(SBrush $cyan),$px,($py+30),$sf)
# HUD (좌상 하트/레이어, 우상 BITS)
$g.FillPolygon((SBrush $pink),@( (New-Object System.Drawing.PointF(34,40)),(New-Object System.Drawing.PointF(46,52)),(New-Object System.Drawing.PointF(34,64)),(New-Object System.Drawing.PointF(22,52)) ))
$g.FillPolygon((SBrush $pink),@( (New-Object System.Drawing.PointF(64,40)),(New-Object System.Drawing.PointF(76,52)),(New-Object System.Drawing.PointF(64,64)),(New-Object System.Drawing.PointF(52,52)) ))
$g.FillPolygon((SBrush $pink),@( (New-Object System.Drawing.PointF(94,40)),(New-Object System.Drawing.PointF(106,52)),(New-Object System.Drawing.PointF(94,64)),(New-Object System.Drawing.PointF(82,52)) ))
$g.DrawString("LAYER 4",$fHint,(SBrush $cyan),24,72)
$g.DrawString("BITS 132",$fHint,(SBrush $amber),($W-150),34)
$g.Dispose()
$dir = Resolve-Path -LiteralPath (Split-Path $Out)
$bmp.Save((Join-Path $dir (Split-Path $Out -Leaf)),[System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output ("saved " + $Out)
