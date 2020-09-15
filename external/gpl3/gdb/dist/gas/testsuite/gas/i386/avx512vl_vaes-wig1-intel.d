#as: -mevexwig=1
#objdump: -dw -Mintel
#name: i386 AVX512VL/VAES wig insns (Intel disassembly)
#source: avx512vl_vaes-wig.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de f4[ 	]*vaesdec xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de b4 f4 c0 1d fe ff[ 	]*vaesdec xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de f4[ 	]*vaesdec ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de b4 f4 c0 1d fe ff[ 	]*vaesdec ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df f4[ 	]*vaesdeclast xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df f4[ 	]*vaesdeclast ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc f4[ 	]*vaesenc xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc b4 f4 c0 1d fe ff[ 	]*vaesenc xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc f4[ 	]*vaesenc ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc b4 f4 c0 1d fe ff[ 	]*vaesenc ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd f4[ 	]*vaesenclast xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd f4[ 	]*vaesenclast ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de f4[ 	]*vaesdec xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de b4 f4 c0 1d fe ff[ 	]*vaesdec xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de f4[ 	]*vaesdec ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de b4 f4 c0 1d fe ff[ 	]*vaesdec ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df f4[ 	]*vaesdeclast xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df f4[ 	]*vaesdeclast ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc f4[ 	]*vaesenc xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc b4 f4 c0 1d fe ff[ 	]*vaesenc xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc f4[ 	]*vaesenc ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc b4 f4 c0 1d fe ff[ 	]*vaesenc ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd f4[ 	]*vaesenclast xmm6,xmm5,xmm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast xmm6,xmm5,XMMWORD PTR \[esp\+esi\*8-0x1e240\]
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd f4[ 	]*vaesenclast ymm6,ymm5,ymm4
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast ymm6,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\]
#pass
