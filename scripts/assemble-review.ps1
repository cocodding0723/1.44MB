# assemble-review.ps1 — ⏳대기 요소 3개 클러스터 컨택트 시트 조립 (rules/50 컨펌 게이트)
# 기존 PNG + 신규 목업을 격자 합성 + 셀 라벨. System.Drawing, 외부 의존 0.
# 출력: docs/design/review-BASE.png, review-B.png, review-NARRATIVE.png
param(
  [string]$Root = "$PSScriptRoot\.."
)

Add-Type -AssemblyName System.Drawing
$design=[System.IO.Path]::GetFullPath("$Root\docs\design")

function SB($r,$gg,$b,$a){ New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($a,$r,$gg,$b)) }
function PN($r,$gg,$b,$a,$w){ New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb($a,$r,$gg,$b),$w) }
$sfC=New-Object System.Drawing.StringFormat; $sfC.Alignment=[System.Drawing.StringAlignment]::Center
$sfCM=New-Object System.Drawing.StringFormat; $sfCM.Alignment=[System.Drawing.StringAlignment]::Center; $sfCM.LineAlignment=[System.Drawing.StringAlignment]::Center

$fTitle=New-Object System.Drawing.Font("Consolas",22,[System.Drawing.FontStyle]::Bold)
$fLabel=New-Object System.Drawing.Font("Consolas",12,[System.Drawing.FontStyle]::Bold)
$fSub=New-Object System.Drawing.Font("Consolas",10,[System.Drawing.FontStyle]::Regular)
$fPlaceholder=New-Object System.Drawing.Font("Consolas",13,[System.Drawing.FontStyle]::Bold)

# 셀 하나 그리기 — 이미지 로드 or 플레이스홀더
function DrawCell($g,$path,$label,$sub,$x,$y,$cw,$ch,$ar,$ag,$ab){
  $imgH=$ch-40  # 이미지 영역 높이

  if($path -and (Test-Path $path)){
    try{
      $src=[System.Drawing.Bitmap]::FromFile($path)
      # 원본 비율 유지하면서 셀 이미지 영역에 맞춤
      $sw=$src.Width; $sh=$src.Height
      $scale=[Math]::Min(($cw-4.0)/$sw, ($imgH-4.0)/$sh)
      $dw=[int]($sw*$scale); $dh=[int]($sh*$scale)
      $dx=$x+($cw-$dw)/2; $dy=$y+($imgH-$dh)/2
      $g.DrawImage($src,[System.Drawing.Rectangle]::new($dx,$dy,$dw,$dh))
      $src.Dispose()
    } catch {
      $g.FillRectangle((SB $ar $ag $ab 20),$x,$y,$cw,$imgH)
      $g.DrawString("LOAD ERR",$fPlaceholder,(SB 255 80 80 200),($x+$cw/2),($y+$imgH/2),$sfCM)
    }
  } else {
    $g.FillRectangle((SB $ar $ag $ab 18),$x,$y,$cw,$imgH)
    $g.DrawRectangle((PN $ar $ag $ab 80 1),$x,$y,$cw,$imgH)
    $g.DrawString("[NOT YET GENERATED]",$fSub,(SB $ar $ag $ab 160),($x+$cw/2),($y+$imgH/2-10),$sfCM)
    $g.DrawString($label,$fPlaceholder,(SB $ar $ag $ab 180),($x+$cw/2),($y+$imgH/2+12),$sfCM)
  }

  # 셀 테두리
  $g.DrawRectangle((PN $ar $ag $ab 120 1),$x,$y,$cw,$imgH)

  # 라벨 바 (하단)
  $lblY=$y+$imgH
  $g.FillRectangle((SB $ar $ag $ab 45),$x,$lblY,$cw,40)
  $g.DrawRectangle((PN $ar $ag $ab 80 1),$x,$lblY,$cw,40)
  $g.DrawString($label,$fLabel,(SB $ar $ag $ab 230),($x+$cw/2),($lblY+3),$sfC)
  if($sub){ $g.DrawString($sub,$fSub,(SB $ar $ag $ab 140),($x+$cw/2),($lblY+22),$sfC) }
}

function MakeSheet($outFile,$title,$cells,$cols,$acR,$acG,$acB){
  $rows=[Math]::Ceiling($cells.Count/$cols)
  $headerH=52; $pad=6; $cellGap=8
  $cw=[int]((1280-$pad*2-($cols-1)*$cellGap)/$cols)
  $ch=200  # 셀 높이(이미지+라벨)
  $totalH=$headerH+$rows*($ch+$cellGap)+$pad

  $bmp=New-Object System.Drawing.Bitmap(1280,$totalH)
  $g=[System.Drawing.Graphics]::FromImage($bmp)
  $g.SmoothingMode=[System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $g.TextRenderingHint=[System.Drawing.Text.TextRenderingHint]::AntiAlias
  $g.Clear([System.Drawing.Color]::FromArgb(255,5,6,12))

  # 헤더
  $g.FillRectangle((SB $acR $acG $acB 12),0,0,1280,$headerH)
  $g.DrawString($title,$fTitle,(SB $acR $acG $acB 225),(1280/2),10,$sfC)
  $g.DrawLine((PN $acR $acG $acB 70 1),30,$headerH,(1280-30),$headerH)

  for($ci=0;$ci -lt $cells.Count;$ci++){
    $col=$ci % $cols; $row=[Math]::Floor($ci/$cols)
    $cx=$pad+$col*($cw+$cellGap)
    $cy=$headerH+$pad+$row*($ch+$cellGap)
    $cell=$cells[$ci]
    $ar=if($cell.ar){$cell.ar}else{$acR}
    $ag=if($cell.ag){$cell.ag}else{$acG}
    $ab=if($cell.ab){$cell.ab}else{$acB}
    DrawCell $g $cell.path $cell.label $cell.sub $cx $cy $cw $ch $ar $ag $ab
  }

  $g.Dispose()
  $absOut=[System.IO.Path]::GetFullPath("$design\$outFile")
  $bmp.Save($absOut,[System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  Write-Output "  saved: $absOut"
  return $absOut
}

# ─────────────────────────────────────────────
# review-BASE.png (11 요소, 4열×3행)
# ─────────────────────────────────────────────
Write-Output "Assembling review-BASE.png ..."
$cyan=@{ar=0;ag=230;ab=255}
$pink=@{ar=255;ag=61;ab=199}
$amber=@{ar=255;ag=171;ab=41}
$green=@{ar=61;ag=255;ab=140}

$baseCells=@(
  @{path="$design\shot-play.png";        label="맵/방 레이아웃";    sub="회로+도트그리드 (v1)"},
  @{path="$design\shots-A\shot-combat.png"; label="플레이어 실루엣"; sub="짐벌 트라이프레임 (v2)"; ar=220;ag=230;ab=240},
  @{path="$design\shots-A\roster-enemies2.png"; label="적 4종 실루엣"; sub="글로우+회전플레이트 (v2)"; ar=255;ag=80;ab=110},
  @{path="$design\shots-A\boss-core.png"; label="보스 WARDEN";   sub="자이로프레임+무장링 (v2)"; ar=255;ag=171;ab=41},
  @{path="$design\shots-A\boss-warden.png"; label="보스 CORE";  sub="동심링+레이저 (v2)"; ar=0;ag=200;ab=255},
  @{path="$design\shot-title.png";       label="UI 타이틀";        sub="NEON DESCENT 타이틀 (v1)"; ar=171;ag=89;ab=255},
  @{path="$design\shot-combat.png";      label="UI HUD";           sub="하트/LAYER/SCORE/미니맵 (v1)"; ar=0;ag=230;ab=255},
  @{path="$design\shot-pause.png";       label="UI 일시정지+옵션"; sub="PAUSE+CRT 토글 (v1)"; ar=61;ag=255;ab=140},
  @{path="$design\module-cards-v1.png";  label="UI 모듈선택";      sub="3장 드래프트 카드 (v1·신규)"; ar=255;ag=171;ab=41},
  @{path="$design\qa\death-3-final.png"; label="UI 게임오버";      sub="SIGNAL LOST 점수분해 (v1)"; ar=255;ag=61;ab=199},
  @{path="$design\palette-v1.png";       label="팰릿 5티어 스와치"; sub="g_palBg/g_palAc §15 (v1·신규)"; ar=171;ag=89;ab=255}
)
# 12번째 빈 슬롯 (4×3=12)
$baseCells += @{path=$null; label="—"; sub="(예비)"; ar=40;ag=50;ab=60}

$baseOut=MakeSheet "review-BASE.png" "NEON DESCENT — BASE v1 디자인 컨펌 (11요소)  2026-06-10 ⏳" $baseCells 4 0 200 255

# ─────────────────────────────────────────────
# review-B.png (4 요소, 2열×2행)
# ─────────────────────────────────────────────
Write-Output "Assembling review-B.png ..."
$bCells=@(
  @{path="$design\shots-B\new-enemies.png";  label="적 LANCER (type5)"; sub="핫핑크 방향 화살촉+돌진 텔레그래프 L5 (v1)"; ar=255;ag=80;ab=110},
  @{path="$design\shots-B\new-enemies2.png"; label="적 WEAVER (type6)"; sub="바이올렛 역회전 헥스 2겹+기뢰 L6 (v1)"; ar=200;ag=80;ab=255},
  @{path="$design\shots-B\nexus-room1.png";  label="보스 THE NEXUS (type2)"; sub="테알 본체+3노드+회전 삼각 빔케이지 L9 (v1)"; ar=0;ag=220;ab=200},
  @{path="$design\module-cards-v1.png";      label="모듈 6종 카드 (B확장)"; sub="VENGEANCE·KINETIC·FRENZY·SIPHON / GC·ARC (v1·신규)"; ar=255;ag=171;ab=41}
)
$bOut=MakeSheet "review-B.png" "NEON DESCENT — B-확장 (신규적2+보스1+모듈6종)  2026-06-13 ⏳" $bCells 2 0 220 200

# ─────────────────────────────────────────────
# review-NARRATIVE.png (4 요소, 2열×2행)
# ─────────────────────────────────────────────
Write-Output "Assembling review-NARRATIVE.png ..."
$nCells=@(
  @{path="$design\shots-narr\v2-comms.png";  label="교신 오버레이"; sub="ECHO 점멸 삼각 시질+노이즈점, 대비0.80 (v2)"; ar=0;ag=230;ab=255},
  @{path="$design\shots-narr\v2-codex.png";  label="코덱스 뷰어"; sub="해금 다이아 마커+텍스트 강조0.92α (v2)"; ar=61;ag=255;ab=140},
  @{path="$design\shots-narr\v2-persona.png";label="페르소나 셀렉터"; sub="REVENANT/DAEMON/SENTINEL/GHOST 시질 (v2)"; ar=171;ag=89;ab=255},
  @{path="$design\shots-narr\ending-merge.png"; label="엔딩 화면 (MERGE 예시)"; sub="4종: PURGE청/MERGE보라/ESCAPE테알/ROT적 (v2)"; ar=200;ag=80;ab=255}
)
$nOut=MakeSheet "review-NARRATIVE.png" "NEON DESCENT — N-서사 내러티브 UI  2026-06-14 ⏳" $nCells 2 171 89 255

Write-Output ""
Write-Output "=== assemble-review 완료 ==="
Write-Output "  BASE    : $baseOut"
Write-Output "  B-확장  : $bOut"
Write-Output "  NARRATIVE: $nOut"
Write-Output "3개 시트를 사용자에게 제시 → 승인 시 CONFIRMATIONS.md ✅ 기록 (PM 담당)"
