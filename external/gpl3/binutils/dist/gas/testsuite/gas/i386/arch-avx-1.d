#as: -march=generic32+avx+aes+pclmul
#objdump: -dw
#name: i386 arch avx 1

.*:     file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	c4 e2 79 dc 11       	vaesenc \(%ecx\),%xmm0,%xmm2
[ 	]*[a-f0-9]+:	c4 e3 49 44 d4 08    	vpclmulqdq \$0x8,%xmm4,%xmm6,%xmm2
#pass
