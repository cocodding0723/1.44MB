# hive-mockup.ps1 — 신규 적 HIVE(소환 노드) 비주얼 목업 (rules/50, 절차 렌더)
param([string]$Out = "docs\design\enemy-hive-v1.png")
Add-Type -AssemblyName System.Drawing
$W=1280; $H=720
$bmp=New-Object System.Drawing.Bitmap($W,$H); $g=[System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::FromArgb(255,8,10,18))
function SB($r,$g2,$b,$a){ New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($a,$r,$g2,$b)) }
function PN($r,$g2,$b,$a,$w){ New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb($a,$r,$g2,$b),$w) }
function Hexagon([double]$cx,[double]$cy,[double]$rad,[double]$rot){ $pts=@(); for($k=0;$k -lt 6;$k++){ $a=$rot+$k*([Math]::PI/3.0); $pts+=,(New-Object System.Drawing.PointF([float]($cx+[Math]::Cos($a)*$rad),[float]($cy+[Math]::Sin($a)*$rad))) }; return [System.Drawing.PointF[]]$pts }
function Tri([double]$cx,[double]$cy,[double]$rad,[double]$rot){ $pts=@(); for($k=0;$k -lt 3;$k++){ $a=$rot+$k*2.0944; $pts+=,(New-Object System.Drawing.PointF([float]($cx+[Math]::Cos($a)*$rad),[float]($cy+[Math]::Sin($a)*$rad))) }; return [System.Drawing.PointF[]]$pts }
# 도트 그리드
$dot=SB 0 230 255 26; for($y=60;$y -lt $H;$y+=44){ for($x=60;$x -lt $W;$x+=44){ $g.FillRectangle($dot,$x,$y,2,2) } }
$g.DrawRectangle((PN 0 230 255 70 2),50,50,($W-100),($H-100))
# 색: 부패 러스트(오렌지-레드) 코어 + 오렌지 글로우
$cr=230;$cg=115;$cb=40; $gr=255;$gg=140;$gb=60
function Hive($cx,$cy,$rad,$birth){  # birth 0~1: 출산 텔레그래프 진행
  $g.FillPolygon((SB $gr $gg $gb 55),(Hexagon $cx $cy ($rad*1.7) 0.2))       # 글로우
  $g.FillPolygon((SB $cr $cg $cb 235),(Hexagon $cx $cy $rad 0.2))           # 외곽 헥스 노드
  $g.DrawPolygon((PN 255 200 120 220 2),[System.Drawing.PointF[]](Hexagon $cx $cy ($rad*0.62) (-0.3))) # 내부 역회전 헥스
  # 출산 코어 (맥동) — birth 클수록 밝게+큰 다이아
  $cs=$rad*(0.30+0.16*$birth)
  $g.FillPolygon((SB 255 235 180 (200+[int](55*$birth))),(Tri $cx $cy $cs 1.0))
  if($birth -gt 0.4){ $g.DrawEllipse((PN 255 220 150 160 2),($cx-$rad*1.1),($cy-$rad*1.1),($rad*2.2),($rad*2.2)) } # 스폰 경고링
}
# 작은 샤드(자손) 그리기
function Shard($cx,$cy,$rad){ $g.FillPolygon((SB 120 230 110 60),(Tri $cx $cy ($rad*1.4) 0.0)); $g.FillPolygon((SB 150 255 140 235),(Tri $cx $cy $rad 0.0)) }
# (1) 평상 HIVE 3개 (출산 단계 다름)
Hive 290 250 34 0.0
Hive 470 250 34 0.55
Hive 650 250 34 0.95
# 막 뱉은 샤드들 (3번째 옆)
Shard 720 215 10; Shard 735 285 9
# (2) 우측: 소환 장면 — HIVE + 방출되는 샤드 궤적
$hx=1000; $hy=440
Hive $hx $hy 36 1.0
Shard ($hx+70) ($hy-40) 11; Shard ($hx+95) ($hy+25) 10; Shard ($hx-60) ($hy+60) 9
$g.DrawLine((PN 255 200 120 110 1),$hx,$hy,($hx+70),($hy-40))
$g.DrawLine((PN 255 200 120 110 1),$hx,$hy,($hx+95),($hy+25))
# 라벨
$fT=New-Object System.Drawing.Font("Consolas",34,[System.Drawing.FontStyle]::Bold)
$fS=New-Object System.Drawing.Font("Consolas",18,[System.Drawing.FontStyle]::Regular)
$sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Center
$g.DrawString("NEW ENEMY  -  HIVE (소환 노드)",$fT,(SB $cr $cg $cb 240),($W/2),70,$sf)
$g.DrawString("회전 헥스 노드 + 맥동 출산 코어 (부패 러스트)",$fS,(SB 220 160 120 220),470,320,$sf)
$g.DrawString("느리고 단단함 → 주기적으로 SHARD 잡몹 방출 (먼저 처치할 우선표적)",$fS,(SB 220 160 120 220),($W/2),560,$sf)
$g.DrawString("HP 36 / 속도 45(느림) / 반경 16 / 3s마다 SHARD 1 (적 수 캡) / DESCENT L8+ · OVERCLOCK t120+",$fS,(SB 150 150 180 200),($W/2),600,$sf)
$g.Dispose()
$dir=Resolve-Path -LiteralPath (Split-Path $Out); $bmp.Save((Join-Path $dir (Split-Path $Out -Leaf)),[System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
Write-Output ("saved "+$Out)
