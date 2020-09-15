#as: -mevexwig=1
#objdump: -dw -Mintel
#name: x86_64 AVX512F/VPCLMULQDQ wig insns (Intel disassembly)
#source: x86-64-avx512f_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 a3 dd 40 44 f3 ab[ 	]*vpclmulqdq zmm22,zmm20,zmm19,0xab
[ 	]*[a-f0-9]+:[ 	]*62 a3 dd 40 44 b4 f0 23 01 00 00 7b[ 	]*vpclmulqdq zmm22,zmm20,ZMMWORD PTR \[rax\+r14\*8\+0x123\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 e3 dd 40 44 72 7f 7b[ 	]*vpclmulqdq zmm22,zmm20,ZMMWORD PTR \[rdx\+0x1fc0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 23 9d 40 44 ef ab[ 	]*vpclmulqdq zmm29,zmm28,zmm23,0xab
[ 	]*[a-f0-9]+:[ 	]*62 23 9d 40 44 ac f0 34 12 00 00 7b[ 	]*vpclmulqdq zmm29,zmm28,ZMMWORD PTR \[rax\+r14\*8\+0x1234\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 63 9d 40 44 6a 7f 7b[ 	]*vpclmulqdq zmm29,zmm28,ZMMWORD PTR \[rdx\+0x1fc0\],0x7b
#pass
