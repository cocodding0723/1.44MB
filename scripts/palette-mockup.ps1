# palette-mockup.ps1 — NEON DESCENT 팰릿 5티어 RGB 스와치 시트 (rules/50, DESIGN §15)
# g_palBg / g_palAc 실제 코드 값(nd_data.inc:254-255) 을 시각화. System.Drawing, 외부 의존 0.
param([string]$Out = "$PSScriptRoot\..\docs\design\palette-v1.png")

Add-Type -AssemblyName System.Drawing
$W=1280; $H=620
$bmp=New-Object System.Drawing.Bitmap($W,$H)
$g=[System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint=[System.Drawing.Text.TextRenderingHint]::AntiAlias
$g.Clear([System.Drawing.Color]::FromArgb(255,4,5,10))

# 5티어 팰릿 데이터 (g_palBg, g_palAc — nd_data.inc 254-255)
# Tier 0=냉각/COOLANT, 1=연산/COMPUTE, 2=메모리/MEMORY, 3=전력/POWER, 4=커널/KERNEL
$tiers = @(
  @{eng="COOLANT"; kor="냉각"; bg=@(8,10,16);  ac=@(0,230,255)},
  @{eng="COMPUTE"; kor="연산"; bg=@(14,8,16);  ac=@(255,61,199)},
  @{eng="MEMORY";  kor="메모리"; bg=@(16,12,6); ac=@(255,171,41)},
  @{eng="POWER";   kor="전력"; bg=@(6,16,10);  ac=@(61,255,140)},
  @{eng="KERNEL";  kor="커널"; bg=@(10,8,18);  ac=@(171,89,255)}
)

function SB($r,$gg,$b,$a){ New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($a,$r,$gg,$b)) }
function PN($r,$gg,$b,$a,$w){ New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb($a,$r,$gg,$b),$w) }
function DrawGlowRect($gx,$x,$y,$ww,$hh,$ar,$ag,$ab){
  for($gi=4;$gi -ge 1;$gi--){
    $ga=[int](18/$gi); $sp=$gi*3
    $gx.FillRectangle((SB $ar $ag $ab $ga),($x-$sp),($y-$sp),($ww+$sp*2),($hh+$sp*2))
  }
  $gx.FillRectangle((SB $ar $ag $ab 255),$x,$y,$ww,$hh)
}
function DrawGlowDiamond($gx,$cx,$cy,$r,$ar,$ag,$ab){
  for($gi=4;$gi -ge 1;$gi--){
    $ga=[int](22/$gi); $sr=$r+$gi*3
    $pts=@((New-Object System.Drawing.PointF($cx,($cy-$sr))),(New-Object System.Drawing.PointF(($cx+$sr),$cy)),(New-Object System.Drawing.PointF($cx,($cy+$sr))),(New-Object System.Drawing.PointF(($cx-$sr),$cy)))
    $gx.FillPolygon((SB $ar $ag $ab $ga),$pts)
  }
  $pts=@((New-Object System.Drawing.PointF($cx,($cy-$r))),(New-Object System.Drawing.PointF(($cx+$r),$cy)),(New-Object System.Drawing.PointF($cx,($cy+$r))),(New-Object System.Drawing.PointF(($cx-$r),$cy)))
  $gx.FillPolygon((SB $ar $ag $ab 255),$pts)
}

$fTitle=New-Object System.Drawing.Font("Consolas",26,[System.Drawing.FontStyle]::Bold)
$fHead=New-Object System.Drawing.Font("Consolas",16,[System.Drawing.FontStyle]::Bold)
$fMono=New-Object System.Drawing.Font("Consolas",12,[System.Drawing.FontStyle]::Regular)
$fSmall=New-Object System.Drawing.Font("Consolas",11,[System.Drawing.FontStyle]::Regular)
$sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Center
$sfL=New-Object System.Drawing.StringFormat; $sfL.Alignment=[System.Drawing.StringAlignment]::Near

# 타이틀 바
$g.FillRectangle((SB 0 200 255 12),0,0,$W,56)
$g.DrawString("NEON DESCENT — PALETTE 5-TIER  [g_palBg / g_palAc  §15]",$fTitle,(SB 0 230 255 230),($W/2),12,$sf)
$g.DrawLine((PN 0 230 255 80 1),40,56,($W-40),56)

# 열 분할 (5열)
$cellW=[int](($W-80)/5); $top=70; $cellH=530; $pad=10
for($i=0;$i -lt 5;$i++){
  $t=$tiers[$i]
  $x=40+$i*$cellW; $cy=$top
  $bg=$t.bg; $ac=$t.ac
  $acCol=[System.Drawing.Color]::FromArgb(255,$ac[0],$ac[1],$ac[2])
  $acBrush=New-Object System.Drawing.SolidBrush($acCol)

  # 셀 테두리 (액센트 글로우)
  $g.DrawRectangle((PN $ac[0] $ac[1] $ac[2] 60 1),($x+3),$cy,($cellW-6),$cellH)
  $g.DrawRectangle((PN $ac[0] $ac[1] $ac[2] 25 3),($x+1),($cy-1),($cellW-2),($cellH+2))

  # 티어 헤더
  $g.DrawString("T$i — $($t.eng)",$fHead,$acBrush,($x+$cellW/2),($cy+10),$sf)
  $g.DrawString("($($t.kor))",$fSmall,(SB $ac[0] $ac[1] $ac[2] 140),($x+$cellW/2),($cy+32),$sf)
  $g.DrawLine((PN $ac[0] $ac[1] $ac[2] 50 1),($x+12),($cy+54),($x+$cellW-12),($cy+54))

  # BG 섹션
  $g.DrawString("BACKGROUND",$fSmall,(SB 160 170 180 200),($x+$cellW/2),($cy+62),$sf)
  # 실제 BG색 (어두워서 8배 증폭한 "가시" 버전과 나란히)
  $bgRaw=[System.Drawing.Color]::FromArgb(255,$bg[0],$bg[1],$bg[2])
  $bgVis=[System.Drawing.Color]::FromArgb(255,[Math]::Min(255,$bg[0]*10+12),[Math]::Min(255,$bg[1]*10+12),[Math]::Min(255,$bg[2]*10+14))
  # 어두운 원본
  $bx=$x+$pad+4; $bw=($cellW-$pad*2-8)/2
  $g.FillRectangle((New-Object System.Drawing.SolidBrush($bgRaw)),$bx,($cy+82),$bw,52)
  $g.DrawRectangle((PN 150 160 170 80 1),$bx,($cy+82),$bw,52)
  $g.DrawString("RAW",$fSmall,(SB 160 170 180 150),($bx+$bw/2),($cy+99),$sf)
  # 증폭 버전
  $bx2=$bx+$bw+4
  $g.FillRectangle((New-Object System.Drawing.SolidBrush($bgVis)),$bx2,($cy+82),($bw-4),52)
  $g.DrawRectangle((PN 150 160 170 80 1),$bx2,($cy+82),($bw-4),52)
  $g.DrawString("×10",$fSmall,(SB 10 10 10 160),($bx2+($bw-4)/2),($cy+99),$sf)
  # RGB 라벨
  $g.DrawString("rgb($($bg[0]),$($bg[1]),$($bg[2]))",$fSmall,(SB 160 170 180 180),($x+$cellW/2),($cy+142),$sf)

  # 구분선
  $g.DrawLine((PN $ac[0] $ac[1] $ac[2] 35 1),($x+12),($cy+162),($x+$cellW-12),($cy+162))

  # ACCENT 섹션
  $g.DrawString("ACCENT",$fSmall,(SB $ac[0] $ac[1] $ac[2] 180),($x+$cellW/2),($cy+170),$sf)
  # 글로우 사각 스와치
  DrawGlowRect $g ($x+$pad+4) ($cy+190) ($cellW-$pad*2-8) 80 $ac[0] $ac[1] $ac[2]
  # 글리프(텍스트) 가독성 확인 — 흰 글자 on 액센트 배경
  $g.DrawString("ACTIVE LAYER",$fSmall,(SB 0 0 0 210),($x+$cellW/2),($cy+222),$sf)
  # RGB 라벨
  $g.DrawString("rgb($($ac[0]),$($ac[1]),$($ac[2]))",$fSmall,$acBrush,($x+$cellW/2),($cy+280),$sf)

  # 구분선
  $g.DrawLine((PN $ac[0] $ac[1] $ac[2] 35 1),($x+12),($cy+302),($x+$cellW-12),($cy+302))

  # 다이아몬드 아이콘 (인게임 요소 표현)
  DrawGlowDiamond $g ($x+$cellW/2) ($cy+365) 30 $ac[0] $ac[1] $ac[2]

  # 탄환 도트 3개 (사격 패턴 암시)
  for($d=0;$d -lt 3;$d++){
    $bdy=$cy+430+$d*20
    DrawGlowDiamond $g ($x+$cellW/2) $bdy 4 $ac[0] $ac[1] $ac[2]
  }

  # 하단 라벨
  $g.DrawString("T$i",$fHead,$acBrush,($x+$cellW/2),($cy+500),$sf)
}

# 저장
$absOut=[System.IO.Path]::GetFullPath($Out)
$dir=Split-Path -Parent $absOut
if(-not (Test-Path $dir)){ New-Item -ItemType Directory -Force $dir | Out-Null }
$bmp.Save($absOut,[System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "palette-mockup saved: $absOut ($W x $H)"
