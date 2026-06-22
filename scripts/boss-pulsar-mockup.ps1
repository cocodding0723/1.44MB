# boss-pulsar-mockup.ps1 — 신규 보스 THE PULSAR (OVERCLOCK 전용) 목업 (rules/50, 절차 렌더)
param([string]$Out = "docs\design\boss-pulsar-v1.png")
Add-Type -AssemblyName System.Drawing
$W=1280; $H=720
$bmp=New-Object System.Drawing.Bitmap($W,$H); $g=[System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::FromArgb(255,8,10,18))
function SB([int]$r,[int]$g2,[int]$b,[int]$a){ New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($a,$r,$g2,$b)) }
function PN([int]$r,[int]$g2,[int]$b,[int]$a,[double]$w){ New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb($a,$r,$g2,$b),[single]$w) }
function Dot([double]$cx,[double]$cy,[double]$rad,$brush){ $g.FillEllipse($brush,[single]($cx-$rad),[single]($cy-$rad),[single]($rad*2),[single]($rad*2)) }
function DiamondPts([double]$cx,[double]$cy,[double]$rad){ return [System.Drawing.PointF[]]@((New-Object System.Drawing.PointF([single]$cx,[single]($cy-$rad))),(New-Object System.Drawing.PointF([single]($cx+$rad),[single]$cy)),(New-Object System.Drawing.PointF([single]$cx,[single]($cy+$rad))),(New-Object System.Drawing.PointF([single]($cx-$rad),[single]$cy))) }
# 도트 그리드
$dot=SB 0 230 255 22; for($y=60;$y -lt $H;$y+=44){ for($x=60;$x -lt $W;$x+=44){ $g.FillRectangle($dot,$x,$y,2,2) } }
$g.DrawRectangle((PN 0 230 255 60 2),50,50,($W-100),($H-100))
$cx=470.0; $cy=380.0   # 보스 중심
# 색: 일렉트릭 옐로-화이트 코어 + 오렌지 글로우/탄
$cr=255;$cg=240;$cb=120; $gr=255;$gg=170;$gb=40; $br=255;$bg=120;$bb=40
# 글로우 동심 + 코어
$g.FillEllipse((SB $gr $gg $gb 45),[single]($cx-130),[single]($cy-130),260,260)
$g.DrawEllipse((PN $gr $gg $gb 90 2),($cx-95),($cy-95),190,190)
$g.DrawEllipse((PN $cr $cg $cb 150 2),($cx-60),($cy-60),120,120)
$g.FillEllipse((SB $cr $cg $cb 240),[single]($cx-38),[single]($cy-38),76,76)
$g.FillEllipse((SB 255 255 255 230),[single]($cx-16),[single]($cy-16),32,32)
# 회전 나선 팔 4개 (코어→외곽, 다이아 점)
for($arm=0;$arm -lt 4;$arm++){
  $base=$arm*1.5708
  for($t=0;$t -lt 9;$t++){
    $rr=40.0+$t*16.0; $ang=$base+$t*0.32
    $ax=$cx+[Math]::Cos($ang)*$rr; $ay=$cy+[Math]::Sin($ang)*$rr
    $g.FillPolygon((SB $gr $gg $gb (220-$t*16)),(DiamondPts $ax $ay (8.0-$t*0.5)))
  }
}
# 확장 탄환 링 (방사 탄, P1 패턴) — 중간 반경 점 24개
for($k=0;$k -lt 24;$k++){ $a=$k*0.2618; $rr=210.0; Dot ($cx+[Math]::Cos($a)*$rr) ($cy+[Math]::Sin($a)*$rr) 6.0 (SB $br $bg $bb 235) }
# 나선 탄 패턴 (P2) — 우측으로 뻗는 나선 점
for($k=0;$k -lt 18;$k++){ $a=$k*0.5; $rr=30.0+$k*9.0; Dot ($cx+[Math]::Cos($a)*$rr) ($cy+[Math]::Sin($a)*$rr) 4.5 (SB 255 200 90 200) }
# 플레이어 (흰 다이아 + 시안 글로우)
$plx=900.0; $ply=560.0
$g.FillPolygon((SB 0 230 255 70),(DiamondPts $plx $ply 22))
$g.FillPolygon((SB 255 255 255 255),(DiamondPts $plx $ply 12))
# 보스 HP 바 (상단)
$g.FillRectangle((SB 40 20 10 200),440,120,400,12)
$g.FillRectangle((SB $gr $gg $gb 235),440,120,300,12)
$g.DrawRectangle((PN $cr $cg $cb 180 1),440,120,400,12)
# 라벨
$fT=New-Object System.Drawing.Font("Consolas",32,[System.Drawing.FontStyle]::Bold)
$fS=New-Object System.Drawing.Font("Consolas",17,[System.Drawing.FontStyle]::Regular)
$sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Center
$g.DrawString("NEW BOSS  -  PULSAR  (OVERCLOCK 전용)",$fT,(SB $cr $cg $cb 240),($W/2),72,$sf)
$g.DrawString("SUBCORE - THE PULSAR",$fS,(SB $cr $cg $cb 200),640,100,$sf)
$g.DrawString("회전 에너지 리액터 — 코어 + 역회전 나선 팔 4개 (일렉트릭 옐로/오렌지)",$fS,(SB 230 200 140 220),($W/2),610,$sf)
$g.DrawString("방사 탄막 보스: P1 확장 탄환 링 → P2 +나선 팔 발사 → P3(격노) 이중 링·고속 회전",$fS,(SB 230 200 140 220),($W/2),645,$sf)
$g.DrawString("OVERCLOCK 보스 웨이브 전용(DESCENT 로테이션·엔딩 불변). spawn_ebul 재활용. 진화=없음(보스)",$fS,(SB 150 150 180 200),($W/2),678,$sf)
$g.Dispose()
$dir=Resolve-Path -LiteralPath (Split-Path $Out); $bmp.Save((Join-Path $dir (Split-Path $Out -Leaf)),[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
Write-Output ("saved "+$Out)
