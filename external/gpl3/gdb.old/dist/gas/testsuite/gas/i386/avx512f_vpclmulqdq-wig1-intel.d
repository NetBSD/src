#as: -mevexwig=1
#objdump: -dw -Mintel
#name: i386 AVX512F/VPCLMULQDQ wig insns (Intel disassembly)
#source: avx512f_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 f2 ab[ 	]*vpclmulqdq zmm6,zmm1,zmm2,0xab
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 b4 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq zmm6,zmm1,ZMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 72 7f 7b[ 	]*vpclmulqdq zmm6,zmm1,ZMMWORD PTR \[edx\+0x1fc0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 ea ab[ 	]*vpclmulqdq zmm5,zmm1,zmm2,0xab
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 ac f4 c0 1d fe ff 7b[ 	]*vpclmulqdq zmm5,zmm1,ZMMWORD PTR \[esp\+esi\*8-0x1e240\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 6a 7f 7b[ 	]*vpclmulqdq zmm5,zmm1,ZMMWORD PTR \[edx\+0x1fc0\],0x7b
#pass
