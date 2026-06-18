# record-oc.ps1 — OVERCLOCK 플레이 영상(애니메이션 GIF) 녹화 (in-session, pwsh7 호환)
# C#은 P/Invoke + 3-3-2 양자화 + 자작 GIF89a/LZW만 (System.Drawing 미사용 → .NET10 Add-Type 호환).
# 비트맵 캡처는 PowerShell 런타임 System.Drawing으로 수행. 게임을 이 세션에서 띄워 포그라운드 유지 → GL 캡처 정상.
param([string]$Out = "docs\design\shots-oc\overclock-gameplay.gif", [int]$Frames = 64, [int]$DelayCs = 6, [int]$SleepMs = 45)

Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies @('System.Collections','System.Runtime','System.Runtime.InteropServices','System.IO.FileSystem','netstandard') @"
using System;
using System.IO;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public class GifEnc {
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
  [DllImport("user32.dll")] public static extern bool PostMessageA(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
  [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
  [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X,Y; }
  public static byte[] Quantize(byte[] bgra, int stride, int w, int h){
    byte[] idx = new byte[w*h];
    for(int y=0;y<h;y++){ int row=y*stride;
      for(int x=0;x<w;x++){ int o=row+x*4; byte b=bgra[o],g=bgra[o+1],r=bgra[o+2];
        idx[y*w+x] = (byte)(((r>>5)<<5)|((g>>5)<<2)|(b>>6)); } }
    return idx;
  }
  List<byte[]> frames = new List<byte[]>();
  public void AddFrame(byte[] idx){ frames.Add(idx); }
  public int Count { get { return frames.Count; } }
  static byte[] Palette(){ byte[] p=new byte[768];
    for(int i=0;i<256;i++){ int r=(i>>5)&7,g=(i>>2)&7,b=i&3; p[i*3]=(byte)(r*255/7); p[i*3+1]=(byte)(g*255/7); p[i*3+2]=(byte)(b*255/3); }
    return p; }
  public void Save(string path, int w, int h, int delayCs){
    using(var fs=new FileStream(path,FileMode.Create))
    using(var bw=new BinaryWriter(fs)){
      bw.Write(new byte[]{71,73,70,56,57,97});
      bw.Write((ushort)w); bw.Write((ushort)h);
      bw.Write((byte)0xF7); bw.Write((byte)0); bw.Write((byte)0);
      bw.Write(Palette());
      bw.Write((byte)0x21); bw.Write((byte)0xFF); bw.Write((byte)11);
      foreach(char c in "NETSCAPE2.0") bw.Write((byte)c);
      bw.Write((byte)3); bw.Write((byte)1); bw.Write((ushort)0); bw.Write((byte)0);
      foreach(var f in frames){
        bw.Write((byte)0x21); bw.Write((byte)0xF9); bw.Write((byte)4);
        bw.Write((byte)0); bw.Write((ushort)delayCs); bw.Write((byte)0); bw.Write((byte)0);
        bw.Write((byte)0x2C); bw.Write((ushort)0); bw.Write((ushort)0); bw.Write((ushort)w); bw.Write((ushort)h); bw.Write((byte)0);
        LZW(bw, f);
      }
      bw.Write((byte)0x3B);
    }
  }
  void LZW(BinaryWriter bw, byte[] data){
    int minCode=8; bw.Write((byte)minCode);
    int clear=1<<minCode, eoi=clear+1, next=eoi+1, codeSize=minCode+1;
    var table=new Dictionary<int,int>();
    var block=new List<byte>(); int bitBuf=0, bitCnt=0;
    Action<int> emit = code => {
      bitBuf |= code << bitCnt; bitCnt += codeSize;
      while(bitCnt>=8){ block.Add((byte)(bitBuf&0xFF)); bitBuf>>=8; bitCnt-=8;
        if(block.Count==255){ bw.Write((byte)255); bw.Write(block.ToArray()); block.Clear(); } }
    };
    emit(clear);
    int prefix = data[0];
    for(int i=1;i<data.Length;i++){
      int k=data[i], key=(prefix<<8)|k; int code;
      if(table.TryGetValue(key,out code)){ prefix=code; }
      else { emit(prefix); table[key]=next++;
        if(next==(1<<codeSize) && codeSize<12) codeSize++;
        prefix=k;
        if(next>=4096){ emit(clear); table.Clear(); next=eoi+1; codeSize=minCode+1; } }
    }
    emit(prefix); emit(eoi);
    if(bitCnt>0){ block.Add((byte)(bitBuf&0xFF)); }
    if(block.Count>0){ bw.Write((byte)block.Count); bw.Write(block.ToArray()); }
    bw.Write((byte)0);
  }
}
"@

# --- 게임 기동 (이 세션 → 포그라운드 유지) ---
Get-Process -Name game -ErrorAction SilentlyContinue | ForEach-Object { $_.Kill() }; Start-Sleep -Milliseconds 400
Start-Process -FilePath "build\game.exe"; Start-Sleep -Milliseconds 1300
$pr = Get-Process -Name game | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if(-not $pr){ throw "game window not found" }
$h = $pr.MainWindowHandle
function K($vk){ [GifEnc]::PostMessageA($h,0x100,[IntPtr]$vk,[IntPtr]0)|Out-Null; Start-Sleep -Milliseconds 55; [GifEnc]::PostMessageA($h,0x101,[IntPtr]$vk,[IntPtr]0)|Out-Null }
K 0x09; Start-Sleep -Milliseconds 300        # TAB -> OVERCLOCK
K 0x20; Start-Sleep -Milliseconds 900        # SPACE -> 시작

# --- 캡처 준비 ---
$rect = New-Object GifEnc+RECT
[GifEnc]::GetClientRect($h, [ref]$rect) | Out-Null
$cw = $rect.R - $rect.L; $ch = $rect.B - $rect.T; $scale = 2
$gw = [int]($cw/$scale); $gh = [int]($ch/$scale)
$enc = New-Object GifEnc
$srect = New-Object System.Drawing.Rectangle 0,0,$gw,$gh
$dirs = @(0x44,0x53,0x41,0x57,0x44,0x57,0x41,0x53); $cd = 0
Write-Output ("DBG client=${cw}x${ch} grab=${gw}x${gh} frames=$Frames")
[GifEnc]::PostMessageA($h,0x100,[IntPtr]$dirs[0],[IntPtr]0) | Out-Null   # hold first dir
for($i=0; $i -lt $Frames; $i++){
  $full = New-Object System.Drawing.Bitmap($cw, $ch)
  $gf = [System.Drawing.Graphics]::FromImage($full)
  $dc = $gf.GetHdc(); [GifEnc]::PrintWindow($h,$dc,2) | Out-Null; $gf.ReleaseHdc($dc)   # DWM 합성 표면(GL 포함) 캡처
  $gf.Dispose()
  $small = New-Object System.Drawing.Bitmap($gw, $gh)
  $gs = [System.Drawing.Graphics]::FromImage($small)
  $gs.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::Bilinear
  $gs.DrawImage($full, 0, 0, $gw, $gh); $gs.Dispose()
  $bd = $small.LockBits($srect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $stride = $bd.Stride
  $buf = New-Object byte[] ($stride*$gh)
  [System.Runtime.InteropServices.Marshal]::Copy($bd.Scan0, $buf, 0, $buf.Length)
  $small.UnlockBits($bd)
  $enc.AddFrame([GifEnc]::Quantize($buf, $stride, $gw, $gh))
  $full.Dispose(); $small.Dispose()
  if($i % 9 -eq 8){ [GifEnc]::PostMessageA($h,0x101,[IntPtr]$dirs[$cd],[IntPtr]0)|Out-Null; $cd=($cd+1)%8; [GifEnc]::PostMessageA($h,0x100,[IntPtr]$dirs[$cd],[IntPtr]0)|Out-Null }
  if($i % 7 -eq 3){ K 0x31 }
  Start-Sleep -Milliseconds $SleepMs
}
[GifEnc]::PostMessageA($h,0x101,[IntPtr]$dirs[$cd],[IntPtr]0) | Out-Null
$dir = Resolve-Path -LiteralPath (Split-Path $Out)
$enc.Save((Join-Path $dir (Split-Path $Out -Leaf)), $gw, $gh, $DelayCs)
Write-Output ("RESULT frames=" + $enc.Count + " -> " + $Out)
Get-Process -Name game -ErrorAction SilentlyContinue | ForEach-Object { $_.Kill() }
