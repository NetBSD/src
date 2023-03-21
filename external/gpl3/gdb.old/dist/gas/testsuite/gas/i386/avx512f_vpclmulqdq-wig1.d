#as: -mevexwig=1
#objdump: -dw
#name: i386 AVX512F/VPCLMULQDQ wig insns
#source: avx512f_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 f2 ab[ 	]*vpclmulqdq \$0xab,%zmm2,%zmm1,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 b4 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm1,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 72 7f 7b[ 	]*vpclmulqdq \$0x7b,0x1fc0\(%edx\),%zmm1,%zmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 ea ab[ 	]*vpclmulqdq \$0xab,%zmm2,%zmm1,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 ac f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%zmm1,%zmm5
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 48 44 6a 7f 7b[ 	]*vpclmulqdq \$0x7b,0x1fc0\(%edx\),%zmm1,%zmm5
#pass
