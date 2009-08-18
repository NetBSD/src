#source: sse-check.s
#as: -msse-check=warning
#stderr: sse-check-warn.e
#objdump: -dw
#name: i386 SSE check (warning)

.*:     file format .*

Disassembly of section .text:

0+ <_start>:
[ 	]*[a-f0-9]+:	0f 58 ca             	addps  %xmm2,%xmm1
[ 	]*[a-f0-9]+:	66 0f 58 ca          	addpd  %xmm2,%xmm1
[ 	]*[a-f0-9]+:	66 0f d0 ca          	addsubpd %xmm2,%xmm1
[ 	]*[a-f0-9]+:	66 0f 38 01 ca       	phaddw %xmm2,%xmm1
[ 	]*[a-f0-9]+:	66 0f 38 15 c1       	blendvpd %xmm0,%xmm1,%xmm0
[ 	]*[a-f0-9]+:	66 0f 38 37 c1       	pcmpgtq %xmm1,%xmm0
#pass
