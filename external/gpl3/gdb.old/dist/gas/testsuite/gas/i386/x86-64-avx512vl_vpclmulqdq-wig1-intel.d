#as: -mevexwig=1
#objdump: -dw -Mintel
#name: x86_64 AVX512VL/VPCLMULQDQ wig insns (Intel disassembly)
#source: x86-64-avx512vl_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 cf ab[ 	]*vpclmulqdq xmm17,xmm21,xmm23,0xab
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 8c f0 23 01 00 00 7b[ 	]*vpclmulqdq xmm17,xmm21,XMMWORD PTR \[rax\+r14\*8\+0x123\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 e3 d5 00 44 4a 7f 7b[ 	]*vpclmulqdq xmm17,xmm21,XMMWORD PTR \[rdx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 fb ab[ 	]*vpclmulqdq ymm23,ymm18,ymm19,0xab
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 bc f0 23 01 00 00 7b[ 	]*vpclmulqdq ymm23,ymm18,YMMWORD PTR \[rax\+r14\*8\+0x123\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 e3 ed 20 44 7a 7f 7b[ 	]*vpclmulqdq ymm23,ymm18,YMMWORD PTR \[rdx\+0xfe0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 cf ab[ 	]*vpclmulqdq xmm17,xmm21,xmm23,0xab
[ 	]*[a-f0-9]+:[ 	]*62 a3 d5 00 44 8c f0 23 01 00 00 7b[ 	]*vpclmulqdq xmm17,xmm21,XMMWORD PTR \[rax\+r14\*8\+0x123\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 e3 d5 00 44 4a 7f 7b[ 	]*vpclmulqdq xmm17,xmm21,XMMWORD PTR \[rdx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 fb ab[ 	]*vpclmulqdq ymm23,ymm18,ymm19,0xab
[ 	]*[a-f0-9]+:[ 	]*62 a3 ed 20 44 bc f0 23 01 00 00 7b[ 	]*vpclmulqdq ymm23,ymm18,YMMWORD PTR \[rax\+r14\*8\+0x123\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 e3 ed 20 44 7a 7f 7b[ 	]*vpclmulqdq ymm23,ymm18,YMMWORD PTR \[rdx\+0xfe0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 d1 ab[ 	]*vpclmulqdq xmm18,xmm22,xmm17,0xab
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq xmm18,xmm22,XMMWORD PTR \[rax\+r14\*8\+0x1234\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 e3 cd 00 44 52 7f 7b[ 	]*vpclmulqdq xmm18,xmm22,XMMWORD PTR \[rdx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 d7 ab[ 	]*vpclmulqdq ymm26,ymm25,ymm23,0xab
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq ymm26,ymm25,YMMWORD PTR \[rax\+r14\*8\+0x1234\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 63 b5 20 44 52 7f 7b[ 	]*vpclmulqdq ymm26,ymm25,YMMWORD PTR \[rdx\+0xfe0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 d1 ab[ 	]*vpclmulqdq xmm18,xmm22,xmm17,0xab
[ 	]*[a-f0-9]+:[ 	]*62 a3 cd 00 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq xmm18,xmm22,XMMWORD PTR \[rax\+r14\*8\+0x1234\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 e3 cd 00 44 52 7f 7b[ 	]*vpclmulqdq xmm18,xmm22,XMMWORD PTR \[rdx\+0x7f0\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 d7 ab[ 	]*vpclmulqdq ymm26,ymm25,ymm23,0xab
[ 	]*[a-f0-9]+:[ 	]*62 23 b5 20 44 94 f0 34 12 00 00 7b[ 	]*vpclmulqdq ymm26,ymm25,YMMWORD PTR \[rax\+r14\*8\+0x1234\],0x7b
[ 	]*[a-f0-9]+:[ 	]*62 63 b5 20 44 52 7f 7b[ 	]*vpclmulqdq ymm26,ymm25,YMMWORD PTR \[rdx\+0xfe0\],0x7b
#pass
