#as: -O0
#objdump: -drw
#name: x86-64 pseudo prefixes

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
 +[a-f0-9]+:	c4 e1 78 28 d7       	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c4 e1 78 28 d7       	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c4 e1 78 29 fa       	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 28 d7          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 28 d7          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 28 d7          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 29 fa          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c4 e1 78 28 10       	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 10          	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 10          	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 10    	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 90 00 00 00 00 	vmovaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 50 00 	vmovaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 90 00 00 00 00 	vmovaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 90 80 00 00 00 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 90 80 00 00 00 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 50 08 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 90 80 00 00 00 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	48 89 c8             	mov    %rcx,%rax
 +[a-f0-9]+:	48 8b c1             	mov    %rcx,%rax
 +[a-f0-9]+:	48 89 c8             	mov    %rcx,%rax
 +[a-f0-9]+:	11 c8                	adc    %ecx,%eax
 +[a-f0-9]+:	13 c1                	adc    %ecx,%eax
 +[a-f0-9]+:	11 c8                	adc    %ecx,%eax
 +[a-f0-9]+:	01 c8                	add    %ecx,%eax
 +[a-f0-9]+:	03 c1                	add    %ecx,%eax
 +[a-f0-9]+:	01 c8                	add    %ecx,%eax
 +[a-f0-9]+:	21 c8                	and    %ecx,%eax
 +[a-f0-9]+:	23 c1                	and    %ecx,%eax
 +[a-f0-9]+:	21 c8                	and    %ecx,%eax
 +[a-f0-9]+:	39 c8                	cmp    %ecx,%eax
 +[a-f0-9]+:	3b c1                	cmp    %ecx,%eax
 +[a-f0-9]+:	39 c8                	cmp    %ecx,%eax
 +[a-f0-9]+:	09 c8                	or     %ecx,%eax
 +[a-f0-9]+:	0b c1                	or     %ecx,%eax
 +[a-f0-9]+:	09 c8                	or     %ecx,%eax
 +[a-f0-9]+:	19 c8                	sbb    %ecx,%eax
 +[a-f0-9]+:	1b c1                	sbb    %ecx,%eax
 +[a-f0-9]+:	19 c8                	sbb    %ecx,%eax
 +[a-f0-9]+:	29 c8                	sub    %ecx,%eax
 +[a-f0-9]+:	2b c1                	sub    %ecx,%eax
 +[a-f0-9]+:	29 c8                	sub    %ecx,%eax
 +[a-f0-9]+:	31 c8                	xor    %ecx,%eax
 +[a-f0-9]+:	33 c1                	xor    %ecx,%eax
 +[a-f0-9]+:	31 c8                	xor    %ecx,%eax
 +[a-f0-9]+:	8b 04 25 78 56 34 12 	mov    0x12345678,%eax
 +[a-f0-9]+:	89 04 25 78 56 34 12 	mov    %eax,0x12345678
 +[a-f0-9]+:	8b 04 25 78 56 34 12 	mov    0x12345678,%eax
 +[a-f0-9]+:	89 04 25 78 56 34 12 	mov    %eax,0x12345678
 +[a-f0-9]+:	a1 f0 de bc 9a 78 56 34 12 	movabs 0x123456789abcdef0,%eax
 +[a-f0-9]+:	a3 f0 de bc 9a 78 56 34 12 	movabs %eax,0x123456789abcdef0
 +[a-f0-9]+:	a1 f0 de bc 9a 78 56 34 12 	movabs 0x123456789abcdef0,%eax
 +[a-f0-9]+:	a3 f0 de bc 9a 78 56 34 12 	movabs %eax,0x123456789abcdef0
 +[a-f0-9]+:	a1 f0 de bc 9a 78 56 34 12 	movabs 0x123456789abcdef0,%eax
 +[a-f0-9]+:	a3 f0 de bc 9a 78 56 34 12 	movabs %eax,0x123456789abcdef0
 +[a-f0-9]+:	a1 f0 de bc 9a 78 56 34 12 	movabs 0x123456789abcdef0,%eax
 +[a-f0-9]+:	a3 f0 de bc 9a 78 56 34 12 	movabs %eax,0x123456789abcdef0
 +[a-f0-9]+:	89 07                	mov    %eax,\(%rdi\)
 +[a-f0-9]+:	8b 07                	mov    \(%rdi\),%eax
 +[a-f0-9]+:	89 07                	mov    %eax,\(%rdi\)
 +[a-f0-9]+:	8b 07                	mov    \(%rdi\),%eax
 +[a-f0-9]+:	8c c7                	mov    %es,%edi
 +[a-f0-9]+:	8e e8                	mov    %eax,%gs
 +[a-f0-9]+:	8c c7                	mov    %es,%edi
 +[a-f0-9]+:	8e e8                	mov    %eax,%gs
 +[a-f0-9]+:	0f 20 c7             	mov    %cr0,%rdi
 +[a-f0-9]+:	0f 22 f8             	mov    %rax,%cr7
 +[a-f0-9]+:	0f 20 c7             	mov    %cr0,%rdi
 +[a-f0-9]+:	0f 22 f8             	mov    %rax,%cr7
 +[a-f0-9]+:	0f 21 c7             	mov    %db0,%rdi
 +[a-f0-9]+:	0f 23 f8             	mov    %rax,%db7
 +[a-f0-9]+:	0f 21 c7             	mov    %db0,%rdi
 +[a-f0-9]+:	0f 23 f8             	mov    %rax,%db7
 +[a-f0-9]+:	c5 f9 93 f8          	kmovb  %k0,%edi
 +[a-f0-9]+:	c5 f9 92 f8          	kmovb  %eax,%k7
 +[a-f0-9]+:	c5 f9 93 f8          	kmovb  %k0,%edi
 +[a-f0-9]+:	c5 f9 92 f8          	kmovb  %eax,%k7
 +[a-f0-9]+:	c5 fb 93 f8          	kmovd  %k0,%edi
 +[a-f0-9]+:	c5 fb 92 f8          	kmovd  %eax,%k7
 +[a-f0-9]+:	c5 fb 93 f8          	kmovd  %k0,%edi
 +[a-f0-9]+:	c5 fb 92 f8          	kmovd  %eax,%k7
 +[a-f0-9]+:	c4 e1 fb 93 f8       	kmovq  %k0,%rdi
 +[a-f0-9]+:	c4 e1 fb 92 f8       	kmovq  %rax,%k7
 +[a-f0-9]+:	c4 e1 fb 93 f8       	kmovq  %k0,%rdi
 +[a-f0-9]+:	c4 e1 fb 92 f8       	kmovq  %rax,%k7
 +[a-f0-9]+:	c5 f8 93 f8          	kmovw  %k0,%edi
 +[a-f0-9]+:	c5 f8 92 f8          	kmovw  %eax,%k7
 +[a-f0-9]+:	c5 f8 93 f8          	kmovw  %k0,%edi
 +[a-f0-9]+:	c5 f8 92 f8          	kmovw  %eax,%k7
 +[a-f0-9]+:	c5 f9 90 f8          	kmovb  %k0,%k7
 +[a-f0-9]+:	c5 f9 90 f8          	kmovb  %k0,%k7
 +[a-f0-9]+:	c4 e1 f9 90 f8       	kmovd  %k0,%k7
 +[a-f0-9]+:	c4 e1 f9 90 f8       	kmovd  %k0,%k7
 +[a-f0-9]+:	c4 e1 f8 90 f8       	kmovq  %k0,%k7
 +[a-f0-9]+:	c4 e1 f8 90 f8       	kmovq  %k0,%k7
 +[a-f0-9]+:	c5 f8 90 f8          	kmovw  %k0,%k7
 +[a-f0-9]+:	c5 f8 90 f8          	kmovw  %k0,%k7
 +[a-f0-9]+:	11 07                	adc    %eax,\(%rdi\)
 +[a-f0-9]+:	13 07                	adc    \(%rdi\),%eax
 +[a-f0-9]+:	11 07                	adc    %eax,\(%rdi\)
 +[a-f0-9]+:	13 07                	adc    \(%rdi\),%eax
 +[a-f0-9]+:	01 07                	add    %eax,\(%rdi\)
 +[a-f0-9]+:	03 07                	add    \(%rdi\),%eax
 +[a-f0-9]+:	01 07                	add    %eax,\(%rdi\)
 +[a-f0-9]+:	03 07                	add    \(%rdi\),%eax
 +[a-f0-9]+:	21 07                	and    %eax,\(%rdi\)
 +[a-f0-9]+:	23 07                	and    \(%rdi\),%eax
 +[a-f0-9]+:	21 07                	and    %eax,\(%rdi\)
 +[a-f0-9]+:	23 07                	and    \(%rdi\),%eax
 +[a-f0-9]+:	39 07                	cmp    %eax,\(%rdi\)
 +[a-f0-9]+:	3b 07                	cmp    \(%rdi\),%eax
 +[a-f0-9]+:	39 07                	cmp    %eax,\(%rdi\)
 +[a-f0-9]+:	3b 07                	cmp    \(%rdi\),%eax
 +[a-f0-9]+:	09 07                	or     %eax,\(%rdi\)
 +[a-f0-9]+:	0b 07                	or     \(%rdi\),%eax
 +[a-f0-9]+:	09 07                	or     %eax,\(%rdi\)
 +[a-f0-9]+:	0b 07                	or     \(%rdi\),%eax
 +[a-f0-9]+:	19 07                	sbb    %eax,\(%rdi\)
 +[a-f0-9]+:	1b 07                	sbb    \(%rdi\),%eax
 +[a-f0-9]+:	19 07                	sbb    %eax,\(%rdi\)
 +[a-f0-9]+:	1b 07                	sbb    \(%rdi\),%eax
 +[a-f0-9]+:	29 07                	sub    %eax,\(%rdi\)
 +[a-f0-9]+:	2b 07                	sub    \(%rdi\),%eax
 +[a-f0-9]+:	29 07                	sub    %eax,\(%rdi\)
 +[a-f0-9]+:	2b 07                	sub    \(%rdi\),%eax
 +[a-f0-9]+:	31 07                	xor    %eax,\(%rdi\)
 +[a-f0-9]+:	33 07                	xor    \(%rdi\),%eax
 +[a-f0-9]+:	31 07                	xor    %eax,\(%rdi\)
 +[a-f0-9]+:	33 07                	xor    \(%rdi\),%eax
 +[a-f0-9]+:	d8 c0                	fadd   %st\(0\),%st
 +[a-f0-9]+:	d8 c0                	fadd   %st\(0\),%st
 +[a-f0-9]+:	dc c0                	fadd   %st,%st\(0\)
 +[a-f0-9]+:	d8 f0                	fdiv   %st\(0\),%st
 +[a-f0-9]+:	d8 f0                	fdiv   %st\(0\),%st
 +[a-f0-9]+:	dc f0                	fdiv   %st,%st\(0\)
 +[a-f0-9]+:	d8 f8                	fdivr  %st\(0\),%st
 +[a-f0-9]+:	d8 f8                	fdivr  %st\(0\),%st
 +[a-f0-9]+:	dc f8                	fdivr  %st,%st\(0\)
 +[a-f0-9]+:	d8 c8                	fmul   %st\(0\),%st
 +[a-f0-9]+:	d8 c8                	fmul   %st\(0\),%st
 +[a-f0-9]+:	dc c8                	fmul   %st,%st\(0\)
 +[a-f0-9]+:	d8 e0                	fsub   %st\(0\),%st
 +[a-f0-9]+:	d8 e0                	fsub   %st\(0\),%st
 +[a-f0-9]+:	dc e0                	fsub   %st,%st\(0\)
 +[a-f0-9]+:	d8 e8                	fsubr  %st\(0\),%st
 +[a-f0-9]+:	d8 e8                	fsubr  %st\(0\),%st
 +[a-f0-9]+:	dc e8                	fsubr  %st,%st\(0\)
 +[a-f0-9]+:	0f 6f f8             	movq   %mm0,%mm7
 +[a-f0-9]+:	0f 6f f8             	movq   %mm0,%mm7
 +[a-f0-9]+:	0f 7f c7             	movq   %mm0,%mm7
 +[a-f0-9]+:	0f 28 f8             	movaps %xmm0,%xmm7
 +[a-f0-9]+:	0f 28 f8             	movaps %xmm0,%xmm7
 +[a-f0-9]+:	0f 29 c7             	movaps %xmm0,%xmm7
 +[a-f0-9]+:	0f 10 f8             	movups %xmm0,%xmm7
 +[a-f0-9]+:	0f 10 f8             	movups %xmm0,%xmm7
 +[a-f0-9]+:	0f 11 c7             	movups %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 10 f8          	movss  %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 10 f8          	movss  %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 11 c7          	movss  %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 28 f8          	movapd %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 28 f8          	movapd %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 29 c7          	movapd %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 10 f8          	movupd %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 10 f8          	movupd %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 11 c7          	movupd %xmm0,%xmm7
 +[a-f0-9]+:	f2 0f 10 f8          	movsd  %xmm0,%xmm7
 +[a-f0-9]+:	f2 0f 10 f8          	movsd  %xmm0,%xmm7
 +[a-f0-9]+:	f2 0f 11 c7          	movsd  %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 6f f8          	movdqa %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 6f f8          	movdqa %xmm0,%xmm7
 +[a-f0-9]+:	66 0f 7f c7          	movdqa %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 6f f8          	movdqu %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 6f f8          	movdqu %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 7f c7          	movdqu %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 7e f8          	movq   %xmm0,%xmm7
 +[a-f0-9]+:	f3 0f 7e f8          	movq   %xmm0,%xmm7
 +[a-f0-9]+:	66 0f d6 c7          	movq   %xmm0,%xmm7
 +[a-f0-9]+:	c5 f8 28 f8          	vmovaps %xmm0,%xmm7
 +[a-f0-9]+:	c5 f8 28 f8          	vmovaps %xmm0,%xmm7
 +[a-f0-9]+:	c5 f8 29 c7          	vmovaps %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7c 48 28 f8    	vmovaps %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7c 48 28 f8    	vmovaps %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7c 48 29 c7    	vmovaps %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7c 0f 28 f8    	vmovaps %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 7c 0f 28 f8    	vmovaps %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 7c 0f 29 c7    	vmovaps %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 7c 48 10 f8    	vmovups %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7c 48 10 f8    	vmovups %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7c 48 11 c7    	vmovups %zmm0,%zmm7
 +[a-f0-9]+:	c5 f8 10 f8          	vmovups %xmm0,%xmm7
 +[a-f0-9]+:	c5 f8 10 f8          	vmovups %xmm0,%xmm7
 +[a-f0-9]+:	c5 f8 11 c7          	vmovups %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7c 0f 10 f8    	vmovups %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 7c 0f 10 f8    	vmovups %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 7c 0f 11 c7    	vmovups %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	c5 f2 10 f8          	vmovss %xmm0,%xmm1,%xmm7
 +[a-f0-9]+:	c5 f2 10 f8          	vmovss %xmm0,%xmm1,%xmm7
 +[a-f0-9]+:	c5 f2 11 c7          	vmovss %xmm0,%xmm1,%xmm7
 +[a-f0-9]+:	62 f1 76 0f 10 f8    	vmovss %xmm0,%xmm1,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 76 0f 10 f8    	vmovss %xmm0,%xmm1,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 76 0f 11 c7    	vmovss %xmm0,%xmm1,%xmm7\{%k7\}
 +[a-f0-9]+:	c5 f9 28 f8          	vmovapd %xmm0,%xmm7
 +[a-f0-9]+:	c5 f9 28 f8          	vmovapd %xmm0,%xmm7
 +[a-f0-9]+:	c5 f9 29 c7          	vmovapd %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fd 48 28 f8    	vmovapd %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 48 28 f8    	vmovapd %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 48 29 c7    	vmovapd %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 0f 28 f8    	vmovapd %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 fd 0f 28 f8    	vmovapd %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 fd 0f 29 c7    	vmovapd %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	c5 f9 10 f8          	vmovupd %xmm0,%xmm7
 +[a-f0-9]+:	c5 f9 10 f8          	vmovupd %xmm0,%xmm7
 +[a-f0-9]+:	c5 f9 11 c7          	vmovupd %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fd 48 10 f8    	vmovupd %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 48 10 f8    	vmovupd %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 48 11 c7    	vmovupd %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 0f 10 f8    	vmovupd %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 fd 0f 10 f8    	vmovupd %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 fd 0f 11 c7    	vmovupd %xmm0,%xmm7\{%k7\}
 +[a-f0-9]+:	c5 f3 10 f8          	vmovsd %xmm0,%xmm1,%xmm7
 +[a-f0-9]+:	c5 f3 10 f8          	vmovsd %xmm0,%xmm1,%xmm7
 +[a-f0-9]+:	c5 f3 11 c7          	vmovsd %xmm0,%xmm1,%xmm7
 +[a-f0-9]+:	62 f1 f7 0f 10 f8    	vmovsd %xmm0,%xmm1,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 f7 0f 10 f8    	vmovsd %xmm0,%xmm1,%xmm7\{%k7\}
 +[a-f0-9]+:	62 f1 f7 0f 11 c7    	vmovsd %xmm0,%xmm1,%xmm7\{%k7\}
 +[a-f0-9]+:	c5 f9 6f f8          	vmovdqa %xmm0,%xmm7
 +[a-f0-9]+:	c5 f9 6f f8          	vmovdqa %xmm0,%xmm7
 +[a-f0-9]+:	c5 f9 7f c7          	vmovdqa %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7d 48 6f f8    	vmovdqa32 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7d 48 6f f8    	vmovdqa32 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7d 48 7f c7    	vmovdqa32 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7d 08 6f f8    	vmovdqa32 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7d 08 6f f8    	vmovdqa32 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7d 08 7f c7    	vmovdqa32 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fd 48 6f f8    	vmovdqa64 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 48 6f f8    	vmovdqa64 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 48 7f c7    	vmovdqa64 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fd 08 6f f8    	vmovdqa64 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fd 08 6f f8    	vmovdqa64 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fd 08 7f c7    	vmovdqa64 %xmm0,%xmm7
 +[a-f0-9]+:	c5 fa 6f f8          	vmovdqu %xmm0,%xmm7
 +[a-f0-9]+:	c5 fa 6f f8          	vmovdqu %xmm0,%xmm7
 +[a-f0-9]+:	c5 fa 7f c7          	vmovdqu %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7f 48 6f f8    	vmovdqu8 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7f 48 6f f8    	vmovdqu8 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7f 48 7f c7    	vmovdqu8 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7f 08 6f f8    	vmovdqu8 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7f 08 6f f8    	vmovdqu8 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7f 48 7f c7    	vmovdqu8 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 ff 48 6f f8    	vmovdqu16 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 ff 48 6f f8    	vmovdqu16 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 ff 48 7f c7    	vmovdqu16 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 ff 08 6f f8    	vmovdqu16 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 ff 08 6f f8    	vmovdqu16 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 ff 08 7f c7    	vmovdqu16 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7e 48 6f f8    	vmovdqu32 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7e 48 6f f8    	vmovdqu32 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7e 48 7f c7    	vmovdqu32 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 7e 08 6f f8    	vmovdqu32 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7e 08 6f f8    	vmovdqu32 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 7e 08 7f c7    	vmovdqu32 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fe 48 6f f8    	vmovdqu64 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fe 48 6f f8    	vmovdqu64 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fe 48 7f c7    	vmovdqu64 %zmm0,%zmm7
 +[a-f0-9]+:	62 f1 fe 08 6f f8    	vmovdqu64 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fe 08 6f f8    	vmovdqu64 %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fe 08 7f c7    	vmovdqu64 %xmm0,%xmm7
 +[a-f0-9]+:	c5 fa 7e f8          	vmovq  %xmm0,%xmm7
 +[a-f0-9]+:	c5 fa 7e f8          	vmovq  %xmm0,%xmm7
 +[a-f0-9]+:	c5 f9 d6 c7          	vmovq  %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fe 08 7e f8    	vmovq  %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fe 08 7e f8    	vmovq  %xmm0,%xmm7
 +[a-f0-9]+:	62 f1 fd 08 d6 c7    	vmovq  %xmm0,%xmm7
 +[a-f0-9]+:	66 0f c5 f8 00       	pextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	66 0f c5 f8 00       	pextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	66 0f 3a 15 c7 00    	pextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	c5 f9 c5 f8 00       	vpextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	c5 f9 c5 f8 00       	vpextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	c4 e3 79 15 c7 00    	vpextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	62 f1 7d 08 c5 f8 00 	vpextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	62 f1 7d 08 c5 f8 00 	vpextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	62 f3 7d 08 15 c7 00 	vpextrw \$0x0,%xmm0,%edi
 +[a-f0-9]+:	66 0f 1a c3          	bndmov %bnd3,%bnd0
 +[a-f0-9]+:	66 0f 1a c3          	bndmov %bnd3,%bnd0
 +[a-f0-9]+:	66 0f 1b d8          	bndmov %bnd3,%bnd0
 +[a-f0-9]+:	0f 28 10             	movaps \(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 10             	movaps \(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 10             	movaps \(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 50 00          	movaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 00 00 00 00 	movaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 50 ff          	movaps -0x1\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 50 ff          	movaps -0x1\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 ff ff ff ff 	movaps -0x1\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 80 00 00 00 	movaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 80 00 00 00 	movaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 80 00 00 00 	movaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	88 c4                	mov    %al,%ah
 +[a-f0-9]+:	40 d3 e0             	rex shl %cl,%eax
 +[a-f0-9]+:	40 a0 01 00 00 00 00 00 00 00 	rex movabs 0x1,%al
 +[a-f0-9]+:	40 38 ca             	rex cmp %cl,%dl
 +[a-f0-9]+:	40 b3 01             	rex mov \$(0x)?1,%bl
 +[a-f0-9]+:	f2 40 0f 38 f0 c1    	rex crc32 %cl,%eax
 +[a-f0-9]+:	40 89 c3             	rex mov %eax,%ebx
 +[a-f0-9]+:	41 89 c6             	mov    %eax,%r14d
 +[a-f0-9]+:	41 89 00             	mov    %eax,\(%r8\)
 +[a-f0-9]+:	40 0f 28 d7          	rex movaps %xmm7,%xmm2
 +[a-f0-9]+:	44 0f 28 e7          	movaps %xmm7,%xmm12
 +[a-f0-9]+:	40 0f 28 11          	rex movaps \(%rcx\),%xmm2
 +[a-f0-9]+:	41 0f 28 10          	movaps \(%r8\),%xmm2
 +[a-f0-9]+:	40 0f 38 01 01       	rex phaddw \(%rcx\),%mm0
 +[a-f0-9]+:	41 0f 38 01 00       	phaddw \(%r8\),%mm0
 +[a-f0-9]+:	8a 45 00             	mov    0x0\(%rbp\),%al
 +[a-f0-9]+:	8a 45 00             	mov    0x0\(%rbp\),%al
 +[a-f0-9]+:	8a 85 00 00 00 00    	mov    0x0\(%rbp\),%al
 +[a-f0-9]+:	67 8a 45 00          	mov    0x0\(%ebp\),%al
 +[a-f0-9]+:	67 8a 45 00          	mov    0x0\(%ebp\),%al
 +[a-f0-9]+:	67 8a 85 00 00 00 00 	mov    0x0\(%ebp\),%al
 +[a-f0-9]+:	41 8a 45 00          	mov    0x0\(%r13\),%al
 +[a-f0-9]+:	41 8a 45 00          	mov    0x0\(%r13\),%al
 +[a-f0-9]+:	41 8a 85 00 00 00 00 	mov    0x0\(%r13\),%al
 +[a-f0-9]+:	67 41 8a 45 00       	mov    0x0\(%r13d\),%al
 +[a-f0-9]+:	67 41 8a 45 00       	mov    0x0\(%r13d\),%al
 +[a-f0-9]+:	67 41 8a 85 00 00 00 00 	mov    0x0\(%r13d\),%al
 +[a-f0-9]+:	c4 e1 78 28 d7       	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c4 e1 78 28 d7       	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c4 e1 78 29 fa       	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 28 d7          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 28 d7          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 28 d7          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c5 f8 29 fa          	vmovaps %xmm7,%xmm2
 +[a-f0-9]+:	c4 e1 78 28 10       	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 10          	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 10          	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 10    	vmovaps \(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 90 00 00 00 00 	vmovaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 50 00 	vmovaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 90 00 00 00 00 	vmovaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 90 80 00 00 00 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	c5 f8 28 90 80 00 00 00 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 50 08 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	62 f1 7c 08 28 90 80 00 00 00 	vmovaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	48 89 c8             	mov    %rcx,%rax
 +[a-f0-9]+:	48 8b c1             	mov    %rcx,%rax
 +[a-f0-9]+:	48 89 c8             	mov    %rcx,%rax
 +[a-f0-9]+:	0f 28 10             	movaps \(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 10             	movaps \(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 10             	movaps \(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 50 00          	movaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 00 00 00 00 	movaps 0x0\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 50 ff          	movaps -0x1\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 50 ff          	movaps -0x1\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 ff ff ff ff 	movaps -0x1\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 80 00 00 00 	movaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 80 00 00 00 	movaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	0f 28 90 80 00 00 00 	movaps 0x80\(%rax\),%xmm2
 +[a-f0-9]+:	88 c4                	mov    %al,%ah
 +[a-f0-9]+:	40 89 c3             	rex mov %eax,%ebx
 +[a-f0-9]+:	41 89 c6             	mov    %eax,%r14d
 +[a-f0-9]+:	41 89 00             	mov    %eax,\(%r8\)
 +[a-f0-9]+:	40 0f 28 d7          	rex movaps %xmm7,%xmm2
 +[a-f0-9]+:	44 0f 28 e7          	movaps %xmm7,%xmm12
 +[a-f0-9]+:	40 0f 28 11          	rex movaps \(%rcx\),%xmm2
 +[a-f0-9]+:	41 0f 28 10          	movaps \(%r8\),%xmm2
 +[a-f0-9]+:	40 0f 38 01 01       	rex phaddw \(%rcx\),%mm0
 +[a-f0-9]+:	41 0f 38 01 00       	phaddw \(%r8\),%mm0
 +[a-f0-9]+:	8a 45 00             	mov    0x0\(%rbp\),%al
 +[a-f0-9]+:	8a 45 00             	mov    0x0\(%rbp\),%al
 +[a-f0-9]+:	8a 85 00 00 00 00    	mov    0x0\(%rbp\),%al
 +[a-f0-9]+:	67 8a 45 00          	mov    0x0\(%ebp\),%al
 +[a-f0-9]+:	67 8a 45 00          	mov    0x0\(%ebp\),%al
 +[a-f0-9]+:	67 8a 85 00 00 00 00 	mov    0x0\(%ebp\),%al
 +[a-f0-9]+:	41 8a 45 00          	mov    0x0\(%r13\),%al
 +[a-f0-9]+:	41 8a 45 00          	mov    0x0\(%r13\),%al
 +[a-f0-9]+:	41 8a 85 00 00 00 00 	mov    0x0\(%r13\),%al
 +[a-f0-9]+:	41 8a 45 00          	mov    0x0\(%r13\),%al
 +[a-f0-9]+:	67 41 8a 45 00       	mov    0x0\(%r13d\),%al
 +[a-f0-9]+:	67 41 8a 85 00 00 00 00 	mov    0x0\(%r13d\),%al
#pass
