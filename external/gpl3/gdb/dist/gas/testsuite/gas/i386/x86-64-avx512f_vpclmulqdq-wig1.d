#as: -mevexwig=1
#objdump: -dw
#name: x86_64 AVX512F/VPCLMULQDQ wig insns
#source: x86-64-avx512f_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

0+ <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 a3 dd 40 44 f3 ab[ 	]*vpclmulqdq \$0xab,%zmm19,%zmm20,%zmm22
[ 	]*[a-f0-9]+:[ 	]*62 a3 dd 40 44 b4 f0 23 01 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x123\(%rax,%r14,8\),%zmm20,%zmm22
[ 	]*[a-f0-9]+:[ 	]*62 e3 dd 40 44 72 7f 7b[ 	]*vpclmulqdq \$0x7b,0x1fc0\(%rdx\),%zmm20,%zmm22
[ 	]*[a-f0-9]+:[ 	]*62 23 9d 40 44 ef ab[ 	]*vpclmulqdq \$0xab,%zmm23,%zmm28,%zmm29
[ 	]*[a-f0-9]+:[ 	]*62 23 9d 40 44 ac f0 34 12 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x1234\(%rax,%r14,8\),%zmm28,%zmm29
[ 	]*[a-f0-9]+:[ 	]*62 63 9d 40 44 6a 7f 7b[ 	]*vpclmulqdq \$0x7b,0x1fc0\(%rdx\),%zmm28,%zmm29
#pass
