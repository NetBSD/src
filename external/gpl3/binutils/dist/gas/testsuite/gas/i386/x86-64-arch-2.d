#as: -march=generic64+avx+vmx+smx+xsave+aes+pclmul+fma+movbe+ept+sse5+3dnowa+svme+padlock
#objdump: -dw
#name: x86-64 arch 2

.*:     file format .*

Disassembly of section .text:

0+ <.text>:
[ 	]*[a-f0-9]+:	0f 44 d8             	cmove  %eax,%ebx
[ 	]*[a-f0-9]+:	0f fc dc             	paddb  %mm4,%mm3
[ 	]*[a-f0-9]+:	f3 0f 58 dc          	addss  %xmm4,%xmm3
[ 	]*[a-f0-9]+:	f2 0f 58 dc          	addsd  %xmm4,%xmm3
[ 	]*[a-f0-9]+:	66 0f d0 dc          	addsubpd %xmm4,%xmm3
[ 	]*[a-f0-9]+:	66 0f 38 01 dc       	phaddw %xmm4,%xmm3
[ 	]*[a-f0-9]+:	66 0f 38 41 d9       	phminposuw %xmm1,%xmm3
[ 	]*[a-f0-9]+:	f2 0f 38 f1 d9       	crc32l %ecx,%ebx
[ 	]*[a-f0-9]+:	c5 fc 77             	vzeroall 
[ 	]*[a-f0-9]+:	0f 01 c4             	vmxoff 
[ 	]*[a-f0-9]+:	0f 37                	getsec 
[ 	]*[a-f0-9]+:	0f 01 d0             	xgetbv 
[ 	]*[a-f0-9]+:	66 0f 38 dc 01       	aesenc \(%rcx\),%xmm0
[ 	]*[a-f0-9]+:	66 0f 3a 44 c1 08    	pclmulqdq \$0x8,%xmm1,%xmm0
[ 	]*[a-f0-9]+:	c4 e2 79 dc 11       	vaesenc \(%rcx\),%xmm0,%xmm2
[ 	]*[a-f0-9]+:	c4 e3 cd 69 fc 20    	vfmaddpd %ymm4,%ymm6,%ymm2,%ymm7
[ 	]*[a-f0-9]+:	0f 38 f0 19          	movbe  \(%rcx\),%ebx
[ 	]*[a-f0-9]+:	66 0f 38 80 19       	invept \(%rcx\),%rbx
[ 	]*[a-f0-9]+:	0f 0f dc b7          	pmulhrw %mm4,%mm3
[ 	]*[a-f0-9]+:	0f 0f dc bb          	pswapd %mm4,%mm3
[ 	]*[a-f0-9]+:	f2 0f 79 ca          	insertq %xmm2,%xmm1
[ 	]*[a-f0-9]+:	0f 01 da             	vmload 
[ 	]*[a-f0-9]+:	f3 0f bd d9          	lzcnt  %ecx,%ebx
[ 	]*[a-f0-9]+:	0f 7a 12 ca          	frczss %xmm2,%xmm1
[ 	]*[a-f0-9]+:	0f a7 c0             	xstore-rng 
#pass
