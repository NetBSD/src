#as: -mevexwig=1
#objdump: -dw
#name: i386 AVX512VL/VPCLMULQDQ wig insns
#source: avx512vl_vpclmulqdq-wig.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*c4 e3 71 44 cc ab[ 	]*vpclmulqdq \$0xab,%xmm4,%xmm1,%xmm1
[ 	]*[a-f0-9]+:[ 	]*c4 e3 71 44 8c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm1,%xmm1
[ 	]*[a-f0-9]+:[ 	]*c4 e3 71 44 8a f0 07 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%edx\),%xmm1,%xmm1
[ 	]*[a-f0-9]+:[ 	]*c4 e3 55 44 da ab[ 	]*vpclmulqdq \$0xab,%ymm2,%ymm5,%ymm3
[ 	]*[a-f0-9]+:[ 	]*c4 e3 55 44 9c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm3
[ 	]*[a-f0-9]+:[ 	]*c4 e3 55 44 9a e0 0f 00 00 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%edx\),%ymm5,%ymm3
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 08 44 cc ab[ 	]*vpclmulqdq \$0xab,%xmm4,%xmm1,%xmm1
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 08 44 8c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm1,%xmm1
[ 	]*[a-f0-9]+:[ 	]*62 f3 f5 08 44 4a 7f 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%edx\),%xmm1,%xmm1
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 28 44 da ab[ 	]*vpclmulqdq \$0xab,%ymm2,%ymm5,%ymm3
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 28 44 9c f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm5,%ymm3
[ 	]*[a-f0-9]+:[ 	]*62 f3 d5 28 44 5a 7f 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%edx\),%ymm5,%ymm3
[ 	]*[a-f0-9]+:[ 	]*c4 e3 59 44 f1 ab[ 	]*vpclmulqdq \$0xab,%xmm1,%xmm4,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e3 59 44 b4 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm4,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e3 59 44 b2 f0 07 00 00 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%edx\),%xmm4,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e3 5d 44 d4 ab[ 	]*vpclmulqdq \$0xab,%ymm4,%ymm4,%ymm2
[ 	]*[a-f0-9]+:[ 	]*c4 e3 5d 44 94 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm4,%ymm2
[ 	]*[a-f0-9]+:[ 	]*c4 e3 5d 44 92 e0 0f 00 00 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%edx\),%ymm4,%ymm2
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 08 44 f1 ab[ 	]*vpclmulqdq \$0xab,%xmm1,%xmm4,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 08 44 b4 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%xmm4,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 08 44 72 7f 7b[ 	]*vpclmulqdq \$0x7b,0x7f0\(%edx\),%xmm4,%xmm6
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 28 44 d4 ab[ 	]*vpclmulqdq \$0xab,%ymm4,%ymm4,%ymm2
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 28 44 94 f4 c0 1d fe ff 7b[ 	]*vpclmulqdq \$0x7b,-0x1e240\(%esp,%esi,8\),%ymm4,%ymm2
[ 	]*[a-f0-9]+:[ 	]*62 f3 dd 28 44 52 7f 7b[ 	]*vpclmulqdq \$0x7b,0xfe0\(%edx\),%ymm4,%ymm2
#pass
