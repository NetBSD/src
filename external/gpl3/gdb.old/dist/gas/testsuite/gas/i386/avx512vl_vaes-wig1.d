#as: -mevexwig=1
#objdump: -dw
#name: i386 AVX512VL/VAES wig insns
#source: avx512vl_vaes-wig.s

.*: +file format .*


Disassembly of section \.text:

00000000 <_start>:
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de f4[ 	]*vaesdec %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de b4 f4 c0 1d fe ff[ 	]*vaesdec -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de f4[ 	]*vaesdec %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de b4 f4 c0 1d fe ff[ 	]*vaesdec -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df f4[ 	]*vaesdeclast %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df f4[ 	]*vaesdeclast %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc f4[ 	]*vaesenc %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc b4 f4 c0 1d fe ff[ 	]*vaesenc -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc f4[ 	]*vaesenc %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc b4 f4 c0 1d fe ff[ 	]*vaesenc -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd f4[ 	]*vaesenclast %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd f4[ 	]*vaesenclast %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de f4[ 	]*vaesdec %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 de b4 f4 c0 1d fe ff[ 	]*vaesdec -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de f4[ 	]*vaesdec %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 de b4 f4 c0 1d fe ff[ 	]*vaesdec -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df f4[ 	]*vaesdeclast %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df f4[ 	]*vaesdeclast %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 df b4 f4 c0 1d fe ff[ 	]*vaesdeclast -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc f4[ 	]*vaesenc %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dc b4 f4 c0 1d fe ff[ 	]*vaesenc -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc f4[ 	]*vaesenc %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dc b4 f4 c0 1d fe ff[ 	]*vaesenc -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd f4[ 	]*vaesenclast %xmm4,%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 51 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast -0x1e240\(%esp,%esi,8\),%xmm5,%xmm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd f4[ 	]*vaesenclast %ymm4,%ymm5,%ymm6
[ 	]*[a-f0-9]+:[ 	]*c4 e2 55 dd b4 f4 c0 1d fe ff[ 	]*vaesenclast -0x1e240\(%esp,%esi,8\),%ymm5,%ymm6
#pass
