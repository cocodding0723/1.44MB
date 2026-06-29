# module-cards-mockup.ps1 — UI 모듈선택 카드 드래프트 + B확장 6종 카드 시트 (rules/50, DESIGN §7)
# 커먼=시안 테두리, 레어=앰버 테두리+★. System.Drawing, 외부 의존 0.
param([string]$Out = "$PSScriptRoot\..\docs\design\module-cards-v1.png")

Add-Type -AssemblyName System.Drawing
$W=1280; $H=900
$bmp=New-Object System.Drawing.Bitmap($W,$H)
$g=[System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint=[System.Drawing.Text.TextRenderingHint]::AntiAlias
$g.Clear([System.Drawing.Color]::FromArgb(255,6,8,14))

function SB($r,$gg,$b,$a){ New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($a,$r,$gg,$b)) }
function PN($r,$gg,$b,$a,$w){ New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb($a,$r,$gg,$b),$w) }
function SF_C{ $sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Center; $sf }
function SF_L{ $sf=New-Object System.Drawing.StringFormat; $sf.Alignment=[System.Drawing.StringAlignment]::Near; $sf }

$sfC=SF_C; $sfL=SF_L
$cyan=@(0,230,255); $pink=@(255,61,199); $amber=@(255,171,41); $green=@(61,255,140); $purple=@(171,89,255)
$white=@(220,230,240)

$fTitle=New-Object System.Drawing.Font("Consolas",24,[System.Drawing.FontStyle]::Bold)
$fSec=New-Object System.Drawing.Font("Consolas",15,[System.Drawing.FontStyle]::Bold)
$fCard=New-Object System.Drawing.Font("Consolas",13,[System.Drawing.FontStyle]::Bold)
$fDesc=New-Object System.Drawing.Font("Consolas",11,[System.Drawing.FontStyle]::Regular)
$fBadge=New-Object System.Drawing.Font("Consolas",10,[System.Drawing.FontStyle]::Bold)
$fTiny=New-Object System.Drawing.Font("Consolas",9,[System.Drawing.FontStyle]::Regular)

function DrawGlowRect($gx,$x,$y,$ww,$hh,$ar,$ag,$ab,$alpha=255){
  for($gi=3;$gi -ge 1;$gi--){
    $ga=[int](16/$gi)
    $gx.FillRectangle((SB $ar $ag $ab $ga),($x-$gi*2),($y-$gi*2),($ww+$gi*4),($hh+$gi*4))
  }
  $gx.DrawRectangle((PN $ar $ag $ab $alpha 2),$x,$y,$ww,$hh)
}

# 아이콘 드로잉 함수들 (각 모듈 특성 표현)
function DrawIcon_Vengeance($gx,$cx,$cy,$ar,$ag,$ab){
  # 아래 화살표 (저HP) + 위 화살표 (뎀업)
  $pts=@((New-Object System.Drawing.PointF($cx,($cy+22))),(New-Object System.Drawing.PointF(($cx-14),($cy+6))),(New-Object System.Drawing.PointF(($cx-6),($cy+6))),(New-Object System.Drawing.PointF(($cx-6),($cy-22))),(New-Object System.Drawing.PointF(($cx+6),($cy-22))),(New-Object System.Drawing.PointF(($cx+6),($cy+6))),(New-Object System.Drawing.PointF(($cx+14),($cy+6))))
  $gx.FillPolygon((SB $ar $ag $ab 200),$pts)
  $gx.DrawPolygon((PN $ar $ag $ab 255 1),$pts)
  # HP 다이아(소)
  $hpts=@((New-Object System.Drawing.PointF($cx,($cy-28))),(New-Object System.Drawing.PointF(($cx+6),($cy-22))),(New-Object System.Drawing.PointF($cx,($cy-16))),(New-Object System.Drawing.PointF(($cx-6),($cy-22))))
  $gx.FillPolygon((SB 255 80 80 200),$hpts)
}
function DrawIcon_Kinetic($gx,$cx,$cy,$ar,$ag,$ab){
  # 이동 화살표 + 속도선
  $pts=@((New-Object System.Drawing.PointF(($cx+22),$cy)),(New-Object System.Drawing.PointF(($cx+8),($cy-12))),(New-Object System.Drawing.PointF(($cx+8),($cy-4))),(New-Object System.Drawing.PointF(($cx-22),($cy-4))),(New-Object System.Drawing.PointF(($cx-22),($cy+4))),(New-Object System.Drawing.PointF(($cx+8),($cy+4))),(New-Object System.Drawing.PointF(($cx+8),($cy+12))))
  $gx.FillPolygon((SB $ar $ag $ab 200),$pts)
  for($li=1;$li -le 3;$li++){ $gx.DrawLine((PN $ar $ag $ab 120 1),($cx-24+$li*3),($cy-8+$li),($cx-24+$li*3-8),($cy-8+$li)) }
}
function DrawIcon_Frenzy($gx,$cx,$cy,$ar,$ag,$ab){
  # 회전 나선 (빠름 표시)
  for($fi=0;$fi -lt 8;$fi++){
    $a=$fi/8.0*6.2832; $ra=10+($fi%2)*8
    $x1=$cx+[Math]::Cos($a)*$ra; $y1=$cy+[Math]::Sin($a)*$ra
    $a2=($fi+1)/8.0*6.2832; $ra2=10+(($fi+1)%2)*8
    $x2=$cx+[Math]::Cos($a2)*$ra2; $y2=$cy+[Math]::Sin($a2)*$ra2
    $gx.DrawLine((PN $ar $ag $ab 220 2),$x1,$y1,$x2,$y2)
  }
  # 중앙 코어
  $gx.FillEllipse((SB $ar $ag $ab 255),($cx-4),($cy-4),8,8)
}
function DrawIcon_Siphon($gx,$cx,$cy,$ar,$ag,$ab){
  # BITS 다이아 + 흡수 파동
  for($si=1;$si -le 3;$si++){
    $gx.DrawEllipse((PN $ar $ag $ab ([int](60/$si)) ([int](1.5+$si/2))),($cx-$si*8),($cy-$si*8),($si*16),($si*16))
  }
  $bpts=@((New-Object System.Drawing.PointF($cx,($cy-10))),(New-Object System.Drawing.PointF(($cx+10),$cy)),(New-Object System.Drawing.PointF($cx,($cy+10))),(New-Object System.Drawing.PointF(($cx-10),$cy)))
  $gx.FillPolygon((SB 255 171 41 240),$bpts)
}
function DrawIcon_GlassCannon($gx,$cx,$cy,$ar,$ag,$ab){
  # 크로스헤어 + 균열 표시
  $gx.DrawEllipse((PN $ar $ag $ab 200 2),($cx-16),($cy-16),32,32)
  $gx.DrawLine((PN $ar $ag $ab 200 2),($cx-22),$cy,($cx-18),$cy)
  $gx.DrawLine((PN $ar $ag $ab 200 2),($cx+18),$cy,($cx+22),$cy)
  $gx.DrawLine((PN $ar $ag $ab 200 2),$cx,($cy-22),$cx,($cy-18))
  $gx.DrawLine((PN $ar $ag $ab 200 2),$cx,($cy+18),$cx,($cy+22))
  # 균열선
  $gx.DrawLine((PN 255 80 80 160 1),($cx-8),($cy-16),($cx+3),($cy+4))
  $gx.DrawLine((PN 255 80 80 160 1),($cx+3),($cy+4),($cx-4),($cy+16))
}
function DrawIcon_ArcChain($gx,$cx,$cy,$ar,$ag,$ab){
  # 아크 체인 (지그재그 번개)
  $pts=@((New-Object System.Drawing.PointF(($cx-20),($cy-14))),(New-Object System.Drawing.PointF(($cx-4),($cy-2))),(New-Object System.Drawing.PointF(($cx-12),$cy)),(New-Object System.Drawing.PointF(($cx+4),($cy+10))),(New-Object System.Drawing.PointF(($cx-2),($cy+12))),(New-Object System.Drawing.PointF(($cx+20),($cy+14))))
  for($ai=0;$ai -lt ($pts.Length-1);$ai++){
    $gx.DrawLine((PN $ar $ag $ab 240 3),$pts[$ai],$pts[$ai+1])
  }
  # 타겟 원(소) — 연쇄 대상
  $gx.DrawEllipse((PN $ar $ag $ab 160 1),($cx+14),($cy+8),10,10)
  $gx.DrawEllipse((PN $ar $ag $ab 120 1),($cx-26),($cy-20),10,10)
}

function DrawCard($gx,$x,$y,$cw,$ch,$name,$desc,$tier,$isRare,$iconFn,$ar,$ag,$ab){
  # 카드 배경
  $gx.FillRectangle((SB $ar $ag $ab 10),$x,$y,$cw,$ch)
  # 카드 테두리 (글로우)
  DrawGlowRect $gx $x $y $cw $ch $ar $ag $ab
  if($isRare){
    $gx.DrawRectangle((PN 255 171 41 40 4),$x,$y,$cw,$ch)
  }

  # RARE/COMMON 배지
  $badgeY=$y+8
  if($isRare){
    $gx.FillRectangle((SB 255 171 41 200),($x+$cw-62),$badgeY,60,20)
    $gx.DrawString("★ RARE",$fBadge,(SB 10 10 10 255),($x+$cw-62+30),($badgeY+2),$sfC)
  } else {
    $gx.FillRectangle((SB $ar $ag $ab 160),($x+$cw-72),$badgeY,70,20)
    $gx.DrawString("COMMON",$fBadge,(SB 10 10 10 255),($x+$cw-72+35),($badgeY+2),$sfC)
  }

  # 아이콘 영역
  $iconY=$y+44
  $gx.FillEllipse((SB $ar $ag $ab 15),($x+$cw/2-30),($iconY),60,60)
  & $iconFn $gx ($x+$cw/2) ($iconY+30) $ar $ag $ab

  # 구분선
  $gx.DrawLine((PN $ar $ag $ab 60 1),($x+12),($y+114),($x+$cw-12),($y+114))

  # 이름
  $gx.DrawString($name,$fCard,(New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255,$ar,$ag,$ab))),($x+$cw/2),($y+120),$sfC)

  # 설명
  $descLines=$desc -split "/"
  $dy=$y+140
  foreach($dl in $descLines){
    $gx.DrawString($dl,$fDesc,(SB 180 190 200 220),($x+$cw/2),$dy,$sfC)
    $dy+=16
  }

  # 티어 하단
  $gx.DrawLine((PN $ar $ag $ab 40 1),($x+12),($y+$ch-30),($x+$cw-12),($y+$ch-30))
  $gx.DrawString($tier,$fTiny,(SB $ar $ag $ab 140),($x+$cw/2),($y+$ch-22),$sfC)
}

# ─── 섹션 1: UI 모듈선택 — 3장 드래프트 레이아웃 ───
$g.FillRectangle((SB 0 230 255 8),0,0,$W,48)
$g.DrawString("MODULE SELECT — 3 CARD DRAFT (DESCENT 모듈방 / OVERCLOCK 레벨업)",$fTitle,(SB 0 230 255 220),($W/2),10,$sfC)
$g.DrawLine((PN 0 230 255 60 1),30,48,($W-30),48)

# 드래프트 카드 3장 (커먼 예시)
$draftCards=@(
  @{name="POWER"; desc="DMG +25%/per stack"; tier="COMMON · MOD_POWER"; ar=$cyan[0]; ag=$cyan[1]; ab=$cyan[2]},
  @{name="AGILITY"; desc="MOVE SPD +12%/per stack"; tier="COMMON · MOD_AGILITY"; ar=$green[0]; ag=$green[1]; ab=$green[2]},
  @{name="EXPLOSIVE"; desc="HITS EXPLODE AoE/per stack"; tier="COMMON · MOD_EXPLODE"; ar=$amber[0]; ag=$amber[1]; ab=$amber[2]}
)
$cardW=340; $cardH=200
$totalW=3*$cardW+2*30; $startX=($W-$totalW)/2
for($ci=0;$ci -lt 3;$ci++){
  $dc=$draftCards[$ci]; $cx=$startX+$ci*($cardW+30); $cy=58
  $g.FillRectangle((SB $dc.ar $dc.ag $dc.ab 10),$cx,$cy,$cardW,$cardH)
  DrawGlowRect $g $cx $cy $cardW $cardH $dc.ar $dc.ag $dc.ab
  # COMMON 배지
  $g.FillRectangle((SB $dc.ar $dc.ag $dc.ab 160),($cx+$cardW-74),($cy+8),72,20)
  $g.DrawString("COMMON",$fBadge,(SB 10 10 10 255),($cx+$cardW-74+36),($cy+10),$sfC)
  # 아이콘 (소형 다이아)
  $dpts=@((New-Object System.Drawing.PointF(($cx+$cardW/2),($cy+32))),(New-Object System.Drawing.PointF(($cx+$cardW/2+20),($cy+55))),(New-Object System.Drawing.PointF(($cx+$cardW/2),($cy+78))),(New-Object System.Drawing.PointF(($cx+$cardW/2-20),($cy+55))))
  $g.FillPolygon((SB $dc.ar $dc.ag $dc.ab 180),$dpts)
  for($gl=3;$gl -ge 1;$gl--){ $ga=[int](20/$gl)
    $pts2=@((New-Object System.Drawing.PointF(($cx+$cardW/2),($cy+32-$gl*2))),(New-Object System.Drawing.PointF(($cx+$cardW/2+20+$gl*2),($cy+55))),(New-Object System.Drawing.PointF(($cx+$cardW/2),($cy+78+$gl*2))),(New-Object System.Drawing.PointF(($cx+$cardW/2-20-$gl*2),($cy+55))))
    $g.FillPolygon((SB $dc.ar $dc.ag $dc.ab $ga),$pts2) }
  $g.DrawLine((PN $dc.ar $dc.ag $dc.ab 50 1),($cx+12),($cy+94),($cx+$cardW-12),($cy+94))
  $g.DrawString($dc.name,$fCard,(New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255,$dc.ar,$dc.ag,$dc.ab))),($cx+$cardW/2),($cy+100),$sfC)
  $g.DrawString($dc.desc,$fDesc,(SB 180 190 200 220),($cx+$cardW/2),($cy+120),$sfC)
  $g.DrawLine((PN $dc.ar $dc.ag $dc.ab 35 1),($cx+12),($cy+148),($cx+$cardW-12),($cy+148))
  $g.DrawString($dc.tier,$fTiny,(SB $dc.ar $dc.ag $dc.ab 120),($cx+$cardW/2),($cy+158),$sfC)
  # 선택 힌트
  $hint=@("[ 1 ]","[ 2 ]","[ 3 ]")
  $g.DrawString($hint[$ci],$fBadge,(SB $dc.ar $dc.ag $dc.ab 180),($cx+$cardW/2),($cy+$cardH-22),$sfC)
}

# ESC 힌트
$g.DrawString("ESC — SKIP   ·   1/2/3 or CLICK — SELECT",$fDesc,(SB 120 130 140 180),($W/2),268,$sfC)

# ─── 구분선 ───
$g.DrawLine((PN 0 230 255 40 1),30,290,($W-30),290)

# ─── 섹션 2: B확장 6종 모듈 카드 ───
$g.DrawString("B-EXPANSION MODULES — 6종 (COMMON×4 + RARE×2)  §7",$fSec,(SB 255 171 41 210),($W/2),300,$sfC)
$g.DrawLine((PN 255 171 41 40 1),30,324,($W-30),324)

$expCards=@(
  @{name="VENGEANCE"; desc="LOW HP = DMG UP/+20% per stack"; tier="COMMON · M_VENGEANCE"; isRare=$false; icon="DrawIcon_Vengeance"; ar=$pink[0]; ag=$pink[1]; ab=$pink[2]},
  @{name="KINETIC"; desc="MOVE SPD = DMG UP/+22% per stack"; tier="COMMON · M_KINETIC"; isRare=$false; icon="DrawIcon_Kinetic"; ar=$green[0]; ag=$green[1]; ab=$green[2]},
  @{name="FRENZY"; desc="HOLD FIRE RAMPS ROF/cap +1.5 per stack"; tier="COMMON · M_FRENZY"; isRare=$false; icon="DrawIcon_Frenzy"; ar=$cyan[0]; ag=$cyan[1]; ab=$cyan[2]},
  @{name="SIPHON"; desc="KILLS DROP BITS/frenzy synergy"; tier="COMMON · M_SIPHON"; isRare=$false; icon="DrawIcon_Siphon"; ar=$amber[0]; ag=$amber[1]; ab=$amber[2]},
  @{name="GLASS CANNON"; desc="+60% DMG per stack/+1 DMG taken cap2"; tier="RARE · M_GLASSCANNON"; isRare=$true; icon="DrawIcon_GlassCannon"; ar=$amber[0]; ag=$amber[1]; ab=$amber[2]},
  @{name="ARC CHAIN"; desc="HIT ARCS to 1 ADJACENT/no recursion"; tier="RARE · M_CHAINLIGHT"; isRare=$true; icon="DrawIcon_ArcChain"; ar=$purple[0]; ag=$purple[1]; ab=$purple[2]}
)

$eCardW=190; $eCardH=210; $eGap=18
$totalEW=6*$eCardW+5*$eGap; $eStartX=($W-$totalEW)/2
for($ei=0;$ei -lt 6;$ei++){
  $ec=$expCards[$ei]; $ex=$eStartX+$ei*($eCardW+$eGap); $ey=334

  # 아이콘 함수 참조
  $iconScript=[ScriptBlock]::Create("param(`$gx,`$cx,`$cy,`$ar,`$ag,`$ab); & $($ec.icon) `$gx `$cx `$cy `$ar `$ag `$ab")

  DrawCard $g $ex $ey $eCardW $eCardH $ec.name $ec.desc $ec.tier $ec.isRare $iconScript $ec.ar $ec.ag $ec.ab
}

# 하단 주석
$g.DrawLine((PN 0 230 255 30 1),30,554,($W-30),554)
$g.DrawString("COMMON: 커먼 드래프트 풀 (모듈방·OC 레벨업)   ·   ★ RARE: 보스 클리어 보상 전용 (DESCENT 한정)",$fTiny,(SB 140 150 160 160),($W/2),562,$sfC)
$g.DrawString("모듈 중복 허용 (캡: GLASS CANNON×2, EXECUTE×3, CRYO×4)  ·  SIPHON↔FRENZY 시너지 확률 가중",$fTiny,(SB 120 130 140 130),($W/2),578,$sfC)

# 저장
$absOut=[System.IO.Path]::GetFullPath($Out)
$dir=Split-Path -Parent $absOut
if(-not (Test-Path $dir)){ New-Item -ItemType Directory -Force $dir | Out-Null }
$bmp.Save($absOut,[System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "module-cards-mockup saved: $absOut ($W x $H)"
