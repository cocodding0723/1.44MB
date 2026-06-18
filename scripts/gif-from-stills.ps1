# gif-from-stills.ps1 — 실제 인게임 스틸 PNG들 → 애니메이션 GIF (하이라이트). System.Drawing 런타임 사용.
param([string]$Out = "docs\design\shots-oc\overclock-highlights.gif", [int]$DelayCs = 110)
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies @('System.Collections','System.Runtime','System.Runtime.InteropServices','System.IO.FileSystem','netstandard') @"
using System;
using System.IO;
using System.Collections.Generic;
public class GifS {
  List<byte[]> frames = new List<byte[]>(); int w, h;
  public GifS(int width,int height){ w=width; h=height; }
  public int Count { get { return frames.Count; } }
  public void AddBGRA(byte[] buf, int stride){
    byte[] idx=new byte[w*h];
    for(int y=0;y<h;y++){ int row=y*stride; for(int x=0;x<w;x++){ int o=row+x*4; byte b=buf[o],g=buf[o+1],r=buf[o+2];
      idx[y*w+x]=(byte)(((r>>5)<<5)|((g>>5)<<2)|(b>>6)); } }
    frames.Add(idx);
  }
  static byte[] Pal(){ byte[] p=new byte[768]; for(int i=0;i<256;i++){ int r=(i>>5)&7,g=(i>>2)&7,b=i&3; p[i*3]=(byte)(r*255/7); p[i*3+1]=(byte)(g*255/7); p[i*3+2]=(byte)(b*255/3); } return p; }
  public void Save(string path,int delayCs){
    using(var fs=new FileStream(path,FileMode.Create)) using(var bw=new BinaryWriter(fs)){
      bw.Write(new byte[]{71,73,70,56,57,97}); bw.Write((ushort)w); bw.Write((ushort)h);
      bw.Write((byte)0xF7); bw.Write((byte)0); bw.Write((byte)0); bw.Write(Pal());
      bw.Write((byte)0x21); bw.Write((byte)0xFF); bw.Write((byte)11); foreach(char c in "NETSCAPE2.0") bw.Write((byte)c);
      bw.Write((byte)3); bw.Write((byte)1); bw.Write((ushort)0); bw.Write((byte)0);
      foreach(var f in frames){ bw.Write((byte)0x21); bw.Write((byte)0xF9); bw.Write((byte)4); bw.Write((byte)0); bw.Write((ushort)delayCs); bw.Write((byte)0); bw.Write((byte)0);
        bw.Write((byte)0x2C); bw.Write((ushort)0); bw.Write((ushort)0); bw.Write((ushort)w); bw.Write((ushort)h); bw.Write((byte)0); LZW(bw,f); }
      bw.Write((byte)0x3B);
    }
  }
  void LZW(BinaryWriter bw, byte[] data){
    int minCode=8; bw.Write((byte)minCode); int clear=1<<minCode, eoi=clear+1, next=eoi+1, codeSize=minCode+1;
    var table=new Dictionary<int,int>(); var block=new List<byte>(); int bb=0,bc=0;
    Action<int> emit = code => { bb|=code<<bc; bc+=codeSize; while(bc>=8){ block.Add((byte)(bb&0xFF)); bb>>=8; bc-=8; if(block.Count==255){ bw.Write((byte)255); bw.Write(block.ToArray()); block.Clear(); } } };
    emit(clear); int prefix=data[0];
    for(int i=1;i<data.Length;i++){ int k=data[i], key=(prefix<<8)|k, code;
      if(table.TryGetValue(key,out code)){ prefix=code; }
      else { emit(prefix);
        if(next>=4096){ emit(clear); table.Clear(); next=eoi+1; codeSize=minCode+1; }
        else { if(next>(1<<codeSize)&&codeSize<12)codeSize++; table[key]=next++; }
        prefix=k; } }
    emit(prefix); emit(eoi); if(bc>0)block.Add((byte)(bb&0xFF)); if(block.Count>0){ bw.Write((byte)block.Count); bw.Write(block.ToArray()); } bw.Write((byte)0);
  }
}
"@
$base = "docs\design\shots-oc"
$seq = @("02-title-overclock.png","03-arena-start.png","04-horde.png","07-leveled.png","06-weapons-active.png")
# 대상 크기 = 첫 이미지의 절반
$first = [System.Drawing.Image]::FromFile((Resolve-Path (Join-Path $base $seq[0])))
$gw = [int]($first.Width/2); $gh = [int]($first.Height/2); $first.Dispose()
$enc = New-Object GifS($gw,$gh)
$srect = New-Object System.Drawing.Rectangle 0,0,$gw,$gh
foreach($fn in $seq){
  $p = Join-Path $base $fn
  if(-not (Test-Path $p)){ Write-Output "skip $fn (missing)"; continue }
  $src = [System.Drawing.Image]::FromFile((Resolve-Path $p))
  $small = New-Object System.Drawing.Bitmap($gw,$gh)
  $g = [System.Drawing.Graphics]::FromImage($small); $g.InterpolationMode=[System.Drawing.Drawing2D.InterpolationMode]::Bilinear
  $g.DrawImage($src,0,0,$gw,$gh); $g.Dispose(); $src.Dispose()
  $bd = $small.LockBits($srect,[System.Drawing.Imaging.ImageLockMode]::ReadOnly,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $stride=$bd.Stride; $buf=New-Object byte[] ($stride*$gh); [System.Runtime.InteropServices.Marshal]::Copy($bd.Scan0,$buf,0,$buf.Length); $small.UnlockBits($bd); $small.Dispose()
  # 같은 프레임을 여러 번 넣어 체류 시간 확보 (DelayCs가 길어 1회로 충분)
  $enc.AddBGRA($buf,$stride)
  Write-Output "added $fn"
}
$dir = Resolve-Path -LiteralPath (Split-Path $Out)
$enc.Save((Join-Path $dir (Split-Path $Out -Leaf)), $DelayCs)
Write-Output ("RESULT frames=" + $enc.Count + " size=${gw}x${gh} -> " + $Out)
