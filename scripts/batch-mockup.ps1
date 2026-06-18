# batch-mockup.ps1 — OVERCLOCK 256적 배칭 렌더 비주얼 목업 (rules/50, docs/07 §14.7)
# 배칭 시 적별 디테일(회전 프레임·글로우 다겹·어픽스 배지)을 단순화 → 256적 60fps. 그 룩을 컨펌용으로 제시.
param([string]$Out = "docs\design\oc-batch-v1.png")
Add-Type -AssemblyName System.Drawing
$W=1280; $H=720
$bmp=New-Object System.Drawing.Bitmap($W,$H); $g=[System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::FromArgb(255,8,12,18))
function SB($r,$g2,$b,$a){ New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($a,$r,$g2,$b)) }
function PN($r,$g2,$b,$a,$w){ New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb($a,$r,$g2,$b),$w) }
# 도트 그리드
$dot=SB 0 230 255 30; for($y=40;$y -lt $H;$y+=44){ for($x=40;$x -lt $W;$x+=44){ $g.FillRectangle($dot,$x,$y,2,2) } }
$g.DrawRectangle((PN 0 230 255 90 2),50,50,($W-100),($H-100))
# 단순화된 적 ~180체 (배치 1패스 = 단색 채움 도형, 디테일 없음). 결정론 LCG.
$seed=12345; function Rnd(){ $script:seed=($script:seed*1103515245+12345) -band 0x7FFFFFFF; return $script:seed }
$types=@(
  @{r=255;g=70;b=110; k='tri'},   # HUNTER 핑크 삼각
  @{r=80;g=190;b=255; k='sq'},    # TURRET 시안 사각
  @{r=255;g=200;b=60; k='dia'},   # RICOCHET 앰버 다이아
  @{r=110;g=255;b=100; k='hex'},  # FORK 그린 헥스
  @{r=120;g=255;b=120; k='tri'}   # SHARD 소형 그린 삼각
)
for($i=0;$i -lt 180;$i++){
  $tx=80+((Rnd) % ($W-160)); $ty=80+((Rnd) % ($H-200))
  $ti=(Rnd) % 5; $t=$types[$ti]; $rad=if($t.k -eq 'sq'){9}elseif($t.k -eq 'hex'){11}else{8}
  $br=SB $t.r $t.g $t.b 235
  switch($t.k){
    'tri'{ $g.FillPolygon($br,@((New-Object System.Drawing.PointF($tx,($ty-$rad))),(New-Object System.Drawing.PointF(($tx+$rad*0.9),($ty+$rad*0.7))),(New-Object System.Drawing.PointF(($tx-$rad*0.9),($ty+$rad*0.7))))) }
    'sq'{ $g.FillRectangle($br,($tx-$rad),($ty-$rad),($rad*2),($rad*2)) }
    'dia'{ $g.FillPolygon($br,@((New-Object System.Drawing.PointF($tx,($ty-$rad))),(New-Object System.Drawing.PointF(($tx+$rad),$ty)),(New-Object System.Drawing.PointF($tx,($ty+$rad))),(New-Object System.Drawing.PointF(($tx-$rad),$ty)))) }
    'hex'{ $pts=@(); for($k=0;$k -lt 6;$k++){ $a=$k/6.0*6.2832; $pts+=New-Object System.Drawing.PointF(($tx+[math]::Cos($a)*$rad),($ty+[math]::Sin($a)*$rad)) }; $g.FillPolygon($br,$pts) }
  }
  # 엘리트 = 단일 외곽선만(배지 생략)
  if(((Rnd) % 12) -eq 0){ $g.DrawEllipse((PN $t.r $t.g $t.b 200 2),($tx-$rad-3),($ty-$rad-3),($rad*2+6),($rad*2+6)) }
}
# 보스 (단순 큰 원)
$g.FillEllipse((SB 255 90 150 60),560,250,160,160); $g.FillEllipse((SB 255 90 150 220),610,300,60,60)
# 플레이어 (흰 다이아 + 시안 글로우)
$px=640;$py=400
$g.FillPolygon((SB 0 230 255 70),@((New-Object System.Drawing.PointF($px,($py-26))),(New-Object System.Drawing.PointF(($px+26),$py)),(New-Object System.Drawing.PointF($px,($py+26))),(New-Object System.Drawing.PointF(($px-26),$py))))
$g.FillPolygon((SB 255 255 255 255),@((New-Object System.Drawing.PointF($px,($py-13))),(New-Object System.Drawing.PointF(($px+13),$py)),(New-Object System.Drawing.PointF($px,($py+13))),(New-Object System.Drawing.PointF(($px-13),$py))))
# HUD + 라벨
$fT=New-Object System.Drawing.Font("Consolas",30,[System.Drawing.FontStyle]::Bold)
$fS=New-Object System.Drawing.Font("Consolas",17,[System.Drawing.FontStyle]::Regular)
$sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Center
$g.DrawString("OVERCLOCK - 256 SWARM (BATCHED)",$fT,(SB 0 230 255 235),($W/2),18,$sf)
$g.DrawString("per-enemy detail simplified (no spin-frame / glow-layers / affix badge) for 60fps @256",$fS,(SB 120 200 230 200),($W/2),58,$sf)
# XP 바 + LV + 타이머
$g.FillRectangle((SB 12 36 22 200),120,92,($W-300),10); $g.FillRectangle((SB 100 255 150 230),120,92,($W-470),10)
$g.DrawString("LV 14",$fS,(SB 120 255 180 235),24,86); $g.DrawString("12:30",$fS,(SB 255 255 255 235),($W-110),84)
$g.Dispose()
$dir=Resolve-Path -LiteralPath (Split-Path $Out); $bmp.Save((Join-Path $dir (Split-Path $Out -Leaf)),[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
Write-Output ("saved "+$Out)
