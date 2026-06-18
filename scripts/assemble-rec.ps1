# assemble-rec.ps1 — rec_frames.bin(raw RGB) → 애니메이션 GIF (System.Drawing 미사용)
param([string]$In = "rec_frames.bin", [string]$Out = "docs\design\shots-oc\overclock-gameplay.gif", [int]$DelayCs = 5)
Add-Type -ReferencedAssemblies @('System.Collections','System.Runtime','System.IO.FileSystem','netstandard') @"
using System;
using System.IO;
using System.Collections.Generic;
public class GifAsm {
  List<byte[]> frames = new List<byte[]>();
  int w, h;
  public GifAsm(int width, int height){ w=width; h=height; }
  public int Count { get { return frames.Count; } }
  public void AddRGB(byte[] src, int off){
    byte[] idx = new byte[w*h];
    for(int i=0;i<w*h;i++){ int o=off+i*3; byte r=src[o],g=src[o+1],b=src[o+2];
      idx[i] = (byte)(((r>>5)<<5)|((g>>5)<<2)|(b>>6)); }
    frames.Add(idx);
  }
  static byte[] Palette(){ byte[] p=new byte[768];
    for(int i=0;i<256;i++){ int r=(i>>5)&7,g=(i>>2)&7,b=i&3; p[i*3]=(byte)(r*255/7); p[i*3+1]=(byte)(g*255/7); p[i*3+2]=(byte)(b*255/3); }
    return p; }
  public void Save(string path, int delayCs){
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
$bytes = [System.IO.File]::ReadAllBytes((Resolve-Path $In))
$dw = [BitConverter]::ToUInt32($bytes,0); $dh = [BitConverter]::ToUInt32($bytes,4); $n = [BitConverter]::ToUInt32($bytes,8)
Write-Output ("header ${dw}x${dh} n=$n total=$($bytes.Length)")
$enc = New-Object GifAsm([int]$dw, [int]$dh)
$fsz = [int]$dw * [int]$dh * 3
for($i=0; $i -lt $n; $i++){ $off = 12 + $i*$fsz; if($off + $fsz -le $bytes.Length){ $enc.AddRGB($bytes, $off) } }
$dir = Resolve-Path -LiteralPath (Split-Path $Out)
$enc.Save((Join-Path $dir (Split-Path $Out -Leaf)), $DelayCs)
Write-Output ("RESULT frames=" + $enc.Count + " -> " + $Out)
