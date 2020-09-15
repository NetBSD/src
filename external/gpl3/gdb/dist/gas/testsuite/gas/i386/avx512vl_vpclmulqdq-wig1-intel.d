#as: -mevexwig=1
#objdump: -dw -Mintel
#name: i386 AVX512VL/VPCLMULQDQ wig insns (Intel disassembly)
#source: avx512vl_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*c4 e3 71 44 cc ab[ 	]*vpclmulqdq xmm1,xmm1,xmm4,0xab
[ 	]*[a-f0-9]+:[ 	]*c4 e3 71 44 8c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq xmm1,xmm1,XMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*c4 e3 71 44 8a f0 07 00 00 7b[ 	]*vpclmulqdq xmm1,xmm1,XMMWORD PTR \[edx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*c4 e3 55 44 da ab[ 	]*vpclmulqdq ymm3,ymm5,ymm2,0xab
[ 	]*[a-f0-9]+:[ 	]*c4 e3 55 44 9c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq ymm3,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*c4 e3 55 44 9a e0 0f 00 00 7b[ 	]*vpclmulqdq ymm3,ymm5,YMMWORD PTR \[edx\+0xfe0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 08 44 cc ab[ 	]*vpclmulqdq xmm1,xmm1,xmm4,0xab
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 08 44 8c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq xmm1,xmm1,XMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 08 44 4a 7f 7b[ 	]*vpclmulqdq xmm1,xmm1,XMMWORD PTR \[edx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 28 44 da ab[ 	]*vpclmulqdq ymm3,ymm5,ymm2,0xab
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 28 44 9c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq ymm3,ymm5,YMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 28 44 5a 7f 7b[ 	]*vpclmulqdq ymm3,ymm5,YMMWORD PTR \[edx\+0xfe0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*c4 e3 59 44 f1 ab[ 	]*vpclmulqdq xmm6,xmm4,xmm1,0xab
[ 	]*[a-f0-9]+:[ 	]*c4 e3 59 44 b4 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq xmm6,xmm4,XMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*c4 e3 59 44 b2 f0 07 00 00 7b[ 	]*vpclmulqdq xmm6,xmm4,XMMWORD PTR \[edx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*c4 e3 5d 44 d4 ab[ 	]*vpclmulqdq ymm2,ymm4,ymm4,0xab
[ 	]*[a-f0-9]+:[ 	]*c4 e3 5d 44 94 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq ymm2,ymm4,YMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*c4 e3 5d 44 92 e0 0f 00 00 7b[ 	]*vpclmulqdq ymm2,ymm4,YMMWORD PTR \[edx\+0xfe0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 08 44 f1 ab[ 	]*vpclmulqdq xmm6,xmm4,xmm1,0xab
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 08 44 b4 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq xmm6,xmm4,XMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 08 44 72 7f 7b[ 	]*vpclmulqdq xmm6,xmm4,XMMWORD PTR \[edx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 28 44 d4 ab[ 	]*vpclmulqdq ymm2,ymm4,ymm4,0xab
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 28 44 94 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq ymm2,ymm4,YMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 28 44 52 7f 7b[ 	]*vpclmulqdq ymm2,ymm4,YMMWORD PTR \[edx\+0xfe0\],0x7b
#pass
