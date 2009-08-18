# SSE5 instructions.
	
	.file	"x86-64-sse5.s"
	.globl	foo
	.type	foo, @function
	.text
	.p2align	5,,31
	.att_syntax
foo:

	fmaddss         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddss         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddss         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddss         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddss         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddss         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmaddsd         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddsd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddsd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddsd         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddsd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddsd         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmaddps         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddps         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddps         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddps         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddps         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddps         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmaddpd         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddpd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddpd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddpd         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddpd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddpd         %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fmsubss         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubss         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubss         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubss         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubss         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubss         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmsubsd         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubsd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubsd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubsd         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubsd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubsd         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmsubps         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubps         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubps         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubps         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubps         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubps         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmsubpd         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubpd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubpd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubpd         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubpd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubpd         %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fnmaddss        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddss        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddss        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddss        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddss        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddss        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmaddsd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddsd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddsd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddsd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddsd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddsd        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmaddps        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddps        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddps        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddps        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddps        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddps        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmaddpd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddpd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddpd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddpd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddpd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddpd        %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fnmsubss        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubss        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubss        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubss        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubss        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubss        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmsubsd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubsd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubsd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubsd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubsd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubsd        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmsubps        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubps        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubps        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubps        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubps        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubps        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmsubpd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubpd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubpd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubpd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubpd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubpd        %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fmaddss         %xmm13, %xmm12, %xmm11, %xmm11
	fmaddss         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmaddss         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmaddss         %xmm11, %xmm13, %xmm12, %xmm11
	fmaddss         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmaddss         %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fmaddsd         %xmm13, %xmm12, %xmm11, %xmm11
	fmaddsd         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmaddsd         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmaddsd         %xmm11, %xmm13, %xmm12, %xmm11
	fmaddsd         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmaddsd         %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fmaddps         %xmm13, %xmm12, %xmm11, %xmm11
	fmaddps         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmaddps         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmaddps         %xmm11, %xmm13, %xmm12, %xmm11
	fmaddps         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmaddps         %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fmaddpd         %xmm13, %xmm12, %xmm11, %xmm11
	fmaddpd         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmaddpd         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmaddpd         %xmm11, %xmm13, %xmm12, %xmm11
	fmaddpd         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmaddpd         %xmm11, %xmm12, 0x100000(%r15), %xmm11

	fmsubss         %xmm13, %xmm12, %xmm11, %xmm11
	fmsubss         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmsubss         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmsubss         %xmm11, %xmm13, %xmm12, %xmm11
	fmsubss         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmsubss         %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fmsubsd         %xmm13, %xmm12, %xmm11, %xmm11
	fmsubsd         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmsubsd         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmsubsd         %xmm11, %xmm13, %xmm12, %xmm11
	fmsubsd         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmsubsd         %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fmsubps         %xmm13, %xmm12, %xmm11, %xmm11
	fmsubps         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmsubps         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmsubps         %xmm11, %xmm13, %xmm12, %xmm11
	fmsubps         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmsubps         %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fmsubpd         %xmm13, %xmm12, %xmm11, %xmm11
	fmsubpd         0x100000(%r15), %xmm12, %xmm11, %xmm11
	fmsubpd         %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fmsubpd         %xmm11, %xmm13, %xmm12, %xmm11
	fmsubpd         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fmsubpd         %xmm11, %xmm12, 0x100000(%r15), %xmm11

	fnmaddss        %xmm13, %xmm12, %xmm11, %xmm11
	fnmaddss        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmaddss        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmaddss        %xmm11, %xmm13, %xmm12, %xmm11
	fnmaddss        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmaddss        %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fnmaddsd        %xmm13, %xmm12, %xmm11, %xmm11
	fnmaddsd        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmaddsd        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmaddsd        %xmm11, %xmm13, %xmm12, %xmm11
	fnmaddsd        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmaddsd        %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fnmaddps        %xmm13, %xmm12, %xmm11, %xmm11
	fnmaddps        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmaddps        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmaddps        %xmm11, %xmm13, %xmm12, %xmm11
	fnmaddps        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmaddps        %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fnmaddpd        %xmm13, %xmm12, %xmm11, %xmm11
	fnmaddpd        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmaddpd        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmaddpd        %xmm11, %xmm13, %xmm12, %xmm11
	fnmaddpd        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmaddpd        %xmm11, %xmm12, 0x100000(%r15), %xmm11

	fnmsubss        %xmm13, %xmm12, %xmm11, %xmm11
	fnmsubss        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmsubss        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmsubss        %xmm11, %xmm13, %xmm12, %xmm11
	fnmsubss        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmsubss        %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fnmsubsd        %xmm13, %xmm12, %xmm11, %xmm11
	fnmsubsd        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmsubsd        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmsubsd        %xmm11, %xmm13, %xmm12, %xmm11
	fnmsubsd        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmsubsd        %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fnmsubps        %xmm13, %xmm12, %xmm11, %xmm11
	fnmsubps        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmsubps        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmsubps        %xmm11, %xmm13, %xmm12, %xmm11
	fnmsubps        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmsubps        %xmm11, %xmm12, 0x100000(%r15), %xmm11
	fnmsubpd        %xmm13, %xmm12, %xmm11, %xmm11
	fnmsubpd        0x100000(%r15), %xmm12, %xmm11, %xmm11
	fnmsubpd        %xmm12, 0x100000(%r15), %xmm11, %xmm11
	fnmsubpd        %xmm11, %xmm13, %xmm12, %xmm11
	fnmsubpd        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	fnmsubpd        %xmm11, %xmm12, 0x100000(%r15), %xmm11

	fmaddss         %xmm3, %xmm12, %xmm1, %xmm1
	fmaddss         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmaddss         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmaddss         %xmm1, %xmm3, %xmm12, %xmm1
	fmaddss         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmaddss         %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fmaddsd         %xmm3, %xmm12, %xmm1, %xmm1
	fmaddsd         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmaddsd         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmaddsd         %xmm1, %xmm3, %xmm12, %xmm1
	fmaddsd         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmaddsd         %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fmaddps         %xmm3, %xmm12, %xmm1, %xmm1
	fmaddps         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmaddps         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmaddps         %xmm1, %xmm3, %xmm12, %xmm1
	fmaddps         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmaddps         %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fmaddpd         %xmm3, %xmm12, %xmm1, %xmm1
	fmaddpd         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmaddpd         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmaddpd         %xmm1, %xmm3, %xmm12, %xmm1
	fmaddpd         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmaddpd         %xmm1, %xmm12, 0x4(%rdx), %xmm1

	fmsubss         %xmm3, %xmm12, %xmm1, %xmm1
	fmsubss         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmsubss         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmsubss         %xmm1, %xmm3, %xmm12, %xmm1
	fmsubss         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmsubss         %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fmsubsd         %xmm3, %xmm12, %xmm1, %xmm1
	fmsubsd         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmsubsd         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmsubsd         %xmm1, %xmm3, %xmm12, %xmm1
	fmsubsd         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmsubsd         %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fmsubps         %xmm3, %xmm12, %xmm1, %xmm1
	fmsubps         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmsubps         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmsubps         %xmm1, %xmm3, %xmm12, %xmm1
	fmsubps         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmsubps         %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fmsubpd         %xmm3, %xmm12, %xmm1, %xmm1
	fmsubpd         0x4(%rdx), %xmm12, %xmm1, %xmm1
	fmsubpd         %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fmsubpd         %xmm1, %xmm3, %xmm12, %xmm1
	fmsubpd         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fmsubpd         %xmm1, %xmm12, 0x4(%rdx), %xmm1

	fnmaddss        %xmm3, %xmm12, %xmm1, %xmm1
	fnmaddss        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmaddss        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmaddss        %xmm1, %xmm3, %xmm12, %xmm1
	fnmaddss        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmaddss        %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fnmaddsd        %xmm3, %xmm12, %xmm1, %xmm1
	fnmaddsd        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmaddsd        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmaddsd        %xmm1, %xmm3, %xmm12, %xmm1
	fnmaddsd        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmaddsd        %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fnmaddps        %xmm3, %xmm12, %xmm1, %xmm1
	fnmaddps        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmaddps        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmaddps        %xmm1, %xmm3, %xmm12, %xmm1
	fnmaddps        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmaddps        %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fnmaddpd        %xmm3, %xmm12, %xmm1, %xmm1
	fnmaddpd        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmaddpd        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmaddpd        %xmm1, %xmm3, %xmm12, %xmm1
	fnmaddpd        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmaddpd        %xmm1, %xmm12, 0x4(%rdx), %xmm1

	fnmsubss        %xmm3, %xmm12, %xmm1, %xmm1
	fnmsubss        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmsubss        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmsubss        %xmm1, %xmm3, %xmm12, %xmm1
	fnmsubss        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmsubss        %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fnmsubsd        %xmm3, %xmm12, %xmm1, %xmm1
	fnmsubsd        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmsubsd        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmsubsd        %xmm1, %xmm3, %xmm12, %xmm1
	fnmsubsd        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmsubsd        %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fnmsubps        %xmm3, %xmm12, %xmm1, %xmm1
	fnmsubps        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmsubps        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmsubps        %xmm1, %xmm3, %xmm12, %xmm1
	fnmsubps        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmsubps        %xmm1, %xmm12, 0x4(%rdx), %xmm1
	fnmsubpd        %xmm3, %xmm12, %xmm1, %xmm1
	fnmsubpd        0x4(%rdx), %xmm12, %xmm1, %xmm1
	fnmsubpd        %xmm12, 0x4(%rdx), %xmm1, %xmm1
	fnmsubpd        %xmm1, %xmm3, %xmm12, %xmm1
	fnmsubpd        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	fnmsubpd        %xmm1, %xmm12, 0x4(%rdx), %xmm1

	fmaddss         %xmm3, %xmm2, %xmm11, %xmm11
	fmaddss         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmaddss         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmaddss         %xmm11, %xmm3, %xmm2, %xmm11
	fmaddss         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmaddss         %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fmaddsd         %xmm3, %xmm2, %xmm11, %xmm11
	fmaddsd         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmaddsd         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmaddsd         %xmm11, %xmm3, %xmm2, %xmm11
	fmaddsd         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmaddsd         %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fmaddps         %xmm3, %xmm2, %xmm11, %xmm11
	fmaddps         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmaddps         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmaddps         %xmm11, %xmm3, %xmm2, %xmm11
	fmaddps         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmaddps         %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fmaddpd         %xmm3, %xmm2, %xmm11, %xmm11
	fmaddpd         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmaddpd         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmaddpd         %xmm11, %xmm3, %xmm2, %xmm11
	fmaddpd         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmaddpd         %xmm11, %xmm2, 0x4(%rdx), %xmm11

	fmsubss         %xmm3, %xmm2, %xmm11, %xmm11
	fmsubss         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmsubss         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmsubss         %xmm11, %xmm3, %xmm2, %xmm11
	fmsubss         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmsubss         %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fmsubsd         %xmm3, %xmm2, %xmm11, %xmm11
	fmsubsd         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmsubsd         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmsubsd         %xmm11, %xmm3, %xmm2, %xmm11
	fmsubsd         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmsubsd         %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fmsubps         %xmm3, %xmm2, %xmm11, %xmm11
	fmsubps         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmsubps         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmsubps         %xmm11, %xmm3, %xmm2, %xmm11
	fmsubps         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmsubps         %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fmsubpd         %xmm3, %xmm2, %xmm11, %xmm11
	fmsubpd         0x4(%rdx), %xmm2, %xmm11, %xmm11
	fmsubpd         %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fmsubpd         %xmm11, %xmm3, %xmm2, %xmm11
	fmsubpd         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fmsubpd         %xmm11, %xmm2, 0x4(%rdx), %xmm11

	fnmaddss        %xmm3, %xmm2, %xmm11, %xmm11
	fnmaddss        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmaddss        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmaddss        %xmm11, %xmm3, %xmm2, %xmm11
	fnmaddss        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmaddss        %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fnmaddsd        %xmm3, %xmm2, %xmm11, %xmm11
	fnmaddsd        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmaddsd        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmaddsd        %xmm11, %xmm3, %xmm2, %xmm11
	fnmaddsd        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmaddsd        %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fnmaddps        %xmm3, %xmm2, %xmm11, %xmm11
	fnmaddps        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmaddps        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmaddps        %xmm11, %xmm3, %xmm2, %xmm11
	fnmaddps        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmaddps        %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fnmaddpd        %xmm3, %xmm2, %xmm11, %xmm11
	fnmaddpd        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmaddpd        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmaddpd        %xmm11, %xmm3, %xmm2, %xmm11
	fnmaddpd        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmaddpd        %xmm11, %xmm2, 0x4(%rdx), %xmm11

	fnmsubss        %xmm3, %xmm2, %xmm11, %xmm11
	fnmsubss        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmsubss        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmsubss        %xmm11, %xmm3, %xmm2, %xmm11
	fnmsubss        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmsubss        %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fnmsubsd        %xmm3, %xmm2, %xmm11, %xmm11
	fnmsubsd        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmsubsd        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmsubsd        %xmm11, %xmm3, %xmm2, %xmm11
	fnmsubsd        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmsubsd        %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fnmsubps        %xmm3, %xmm2, %xmm11, %xmm11
	fnmsubps        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmsubps        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmsubps        %xmm11, %xmm3, %xmm2, %xmm11
	fnmsubps        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmsubps        %xmm11, %xmm2, 0x4(%rdx), %xmm11
	fnmsubpd        %xmm3, %xmm2, %xmm11, %xmm11
	fnmsubpd        0x4(%rdx), %xmm2, %xmm11, %xmm11
	fnmsubpd        %xmm2, 0x4(%rdx), %xmm11, %xmm11
	fnmsubpd        %xmm11, %xmm3, %xmm2, %xmm11
	fnmsubpd        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	fnmsubpd        %xmm11, %xmm2, 0x4(%rdx), %xmm11

	fmaddss         %xmm13, %xmm2, %xmm1, %xmm1
	fmaddss         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddss         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddss         %xmm1, %xmm13, %xmm2, %xmm1
	fmaddss         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddss         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmaddsd         %xmm13, %xmm2, %xmm1, %xmm1
	fmaddsd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddsd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddsd         %xmm1, %xmm13, %xmm2, %xmm1
	fmaddsd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddsd         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmaddps         %xmm13, %xmm2, %xmm1, %xmm1
	fmaddps         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddps         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddps         %xmm1, %xmm13, %xmm2, %xmm1
	fmaddps         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddps         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmaddpd         %xmm13, %xmm2, %xmm1, %xmm1
	fmaddpd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmaddpd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmaddpd         %xmm1, %xmm13, %xmm2, %xmm1
	fmaddpd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmaddpd         %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fmsubss         %xmm13, %xmm2, %xmm1, %xmm1
	fmsubss         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubss         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubss         %xmm1, %xmm13, %xmm2, %xmm1
	fmsubss         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubss         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmsubsd         %xmm13, %xmm2, %xmm1, %xmm1
	fmsubsd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubsd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubsd         %xmm1, %xmm13, %xmm2, %xmm1
	fmsubsd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubsd         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmsubps         %xmm13, %xmm2, %xmm1, %xmm1
	fmsubps         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubps         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubps         %xmm1, %xmm13, %xmm2, %xmm1
	fmsubps         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubps         %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fmsubpd         %xmm13, %xmm2, %xmm1, %xmm1
	fmsubpd         0x4(%rdx), %xmm2, %xmm1, %xmm1
	fmsubpd         %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fmsubpd         %xmm1, %xmm13, %xmm2, %xmm1
	fmsubpd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fmsubpd         %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fnmaddss        %xmm13, %xmm2, %xmm1, %xmm1
	fnmaddss        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddss        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddss        %xmm1, %xmm13, %xmm2, %xmm1
	fnmaddss        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddss        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmaddsd        %xmm13, %xmm2, %xmm1, %xmm1
	fnmaddsd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddsd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddsd        %xmm1, %xmm13, %xmm2, %xmm1
	fnmaddsd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddsd        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmaddps        %xmm13, %xmm2, %xmm1, %xmm1
	fnmaddps        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddps        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddps        %xmm1, %xmm13, %xmm2, %xmm1
	fnmaddps        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddps        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmaddpd        %xmm13, %xmm2, %xmm1, %xmm1
	fnmaddpd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmaddpd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmaddpd        %xmm1, %xmm13, %xmm2, %xmm1
	fnmaddpd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmaddpd        %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fnmsubss        %xmm13, %xmm2, %xmm1, %xmm1
	fnmsubss        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubss        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubss        %xmm1, %xmm13, %xmm2, %xmm1
	fnmsubss        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubss        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmsubsd        %xmm13, %xmm2, %xmm1, %xmm1
	fnmsubsd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubsd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubsd        %xmm1, %xmm13, %xmm2, %xmm1
	fnmsubsd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubsd        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmsubps        %xmm13, %xmm2, %xmm1, %xmm1
	fnmsubps        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubps        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubps        %xmm1, %xmm13, %xmm2, %xmm1
	fnmsubps        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubps        %xmm1, %xmm2, 0x4(%rdx), %xmm1
	fnmsubpd        %xmm13, %xmm2, %xmm1, %xmm1
	fnmsubpd        0x4(%rdx), %xmm2, %xmm1, %xmm1
	fnmsubpd        %xmm2, 0x4(%rdx), %xmm1, %xmm1
	fnmsubpd        %xmm1, %xmm13, %xmm2, %xmm1
	fnmsubpd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	fnmsubpd        %xmm1, %xmm2, 0x4(%rdx), %xmm1

	fmaddss         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddss         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmaddss         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmaddss         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddss         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmaddss         %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fmaddsd         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddsd         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmaddsd         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmaddsd         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddsd         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmaddsd         %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fmaddps         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddps         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmaddps         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmaddps         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddps         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmaddps         %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fmaddpd         %xmm3, %xmm2, %xmm1, %xmm1
	fmaddpd         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmaddpd         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmaddpd         %xmm1, %xmm3, %xmm2, %xmm1
	fmaddpd         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmaddpd         %xmm1, %xmm2, 0x100000(%r15), %xmm1

	fmsubss         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubss         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmsubss         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmsubss         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubss         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmsubss         %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fmsubsd         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubsd         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmsubsd         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmsubsd         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubsd         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmsubsd         %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fmsubps         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubps         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmsubps         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmsubps         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubps         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmsubps         %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fmsubpd         %xmm3, %xmm2, %xmm1, %xmm1
	fmsubpd         0x100000(%r15), %xmm2, %xmm1, %xmm1
	fmsubpd         %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fmsubpd         %xmm1, %xmm3, %xmm2, %xmm1
	fmsubpd         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fmsubpd         %xmm1, %xmm2, 0x100000(%r15), %xmm1

	fnmaddss        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddss        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmaddss        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmaddss        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddss        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmaddss        %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fnmaddsd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddsd        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmaddsd        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmaddsd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddsd        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmaddsd        %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fnmaddps        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddps        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmaddps        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmaddps        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddps        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmaddps        %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fnmaddpd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmaddpd        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmaddpd        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmaddpd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmaddpd        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmaddpd        %xmm1, %xmm2, 0x100000(%r15), %xmm1

	fnmsubss        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubss        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmsubss        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmsubss        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubss        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmsubss        %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fnmsubsd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubsd        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmsubsd        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmsubsd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubsd        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmsubsd        %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fnmsubps        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubps        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmsubps        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmsubps        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubps        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmsubps        %xmm1, %xmm2, 0x100000(%r15), %xmm1
	fnmsubpd        %xmm3, %xmm2, %xmm1, %xmm1
	fnmsubpd        0x100000(%r15), %xmm2, %xmm1, %xmm1
	fnmsubpd        %xmm2, 0x100000(%r15), %xmm1, %xmm1
	fnmsubpd        %xmm1, %xmm3, %xmm2, %xmm1
	fnmsubpd        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	fnmsubpd        %xmm1, %xmm2, 0x100000(%r15), %xmm1

	pmacssww        %xmm1, %xmm3, %xmm2, %xmm1
	pmacssww        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsww         %xmm1, %xmm3, %xmm2, %xmm1
	pmacsww         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacswd         %xmm1, %xmm3, %xmm2, %xmm1
	pmacswd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacssdd        %xmm1, %xmm3, %xmm2, %xmm1
	pmacssdd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsdd         %xmm1, %xmm3, %xmm2, %xmm1
	pmacsdd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacssdql       %xmm1, %xmm3, %xmm2, %xmm1
	pmacssdql       %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacssdqh       %xmm1, %xmm3, %xmm2, %xmm1
	pmacssdqh       %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsdql        %xmm1, %xmm3, %xmm2, %xmm1
	pmacsdql        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsdqh        %xmm1, %xmm3, %xmm2, %xmm1
	pmacsdqh        %xmm1, 0x4(%rdx), %xmm2, %xmm1

	pmadcsswd       %xmm1, %xmm3, %xmm2, %xmm1
	pmadcsswd       %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmadcswd        %xmm1, %xmm3, %xmm2, %xmm1
	pmadcswd        %xmm1, 0x4(%rdx), %xmm2, %xmm1

	pmacssww        %xmm11, %xmm13, %xmm12, %xmm11
	pmacssww        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacsww         %xmm11, %xmm13, %xmm12, %xmm11
	pmacsww         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacswd         %xmm11, %xmm13, %xmm12, %xmm11
	pmacswd         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacssdd        %xmm11, %xmm13, %xmm12, %xmm11
	pmacssdd        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacsdd         %xmm11, %xmm13, %xmm12, %xmm11
	pmacsdd         %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacssdql       %xmm11, %xmm13, %xmm12, %xmm11
	pmacssdql       %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacssdqh       %xmm11, %xmm13, %xmm12, %xmm11
	pmacssdqh       %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacsdql        %xmm11, %xmm13, %xmm12, %xmm11
	pmacsdql        %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmacsdqh        %xmm11, %xmm13, %xmm12, %xmm11
	pmacsdqh        %xmm11, 0x100000(%r15), %xmm12, %xmm11

	pmadcsswd       %xmm11, %xmm13, %xmm12, %xmm11
	pmadcsswd       %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pmadcswd        %xmm11, %xmm13, %xmm12, %xmm11
	pmadcswd        %xmm11, 0x100000(%r15), %xmm12, %xmm11

	pmacssww        %xmm1, %xmm3, %xmm12, %xmm1
	pmacssww        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacsww         %xmm1, %xmm3, %xmm12, %xmm1
	pmacsww         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacswd         %xmm1, %xmm3, %xmm12, %xmm1
	pmacswd         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacssdd        %xmm1, %xmm3, %xmm12, %xmm1
	pmacssdd        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacsdd         %xmm1, %xmm3, %xmm12, %xmm1
	pmacsdd         %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacssdql       %xmm1, %xmm3, %xmm12, %xmm1
	pmacssdql       %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacssdqh       %xmm1, %xmm3, %xmm12, %xmm1
	pmacssdqh       %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacsdql        %xmm1, %xmm3, %xmm12, %xmm1
	pmacsdql        %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmacsdqh        %xmm1, %xmm3, %xmm12, %xmm1
	pmacsdqh        %xmm1, 0x4(%rdx), %xmm12, %xmm1

	pmadcsswd       %xmm1, %xmm3, %xmm12, %xmm1
	pmadcsswd       %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pmadcswd        %xmm1, %xmm3, %xmm12, %xmm1
	pmadcswd        %xmm1, 0x4(%rdx), %xmm12, %xmm1

	pmacssww        %xmm11, %xmm3, %xmm2, %xmm11
	pmacssww        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacsww         %xmm11, %xmm3, %xmm2, %xmm11
	pmacsww         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacswd         %xmm11, %xmm3, %xmm2, %xmm11
	pmacswd         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacssdd        %xmm11, %xmm3, %xmm2, %xmm11
	pmacssdd        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacsdd         %xmm11, %xmm3, %xmm2, %xmm11
	pmacsdd         %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacssdql       %xmm11, %xmm3, %xmm2, %xmm11
	pmacssdql       %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacssdqh       %xmm11, %xmm3, %xmm2, %xmm11
	pmacssdqh       %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacsdql        %xmm11, %xmm3, %xmm2, %xmm11
	pmacsdql        %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmacsdqh        %xmm11, %xmm3, %xmm2, %xmm11
	pmacsdqh        %xmm11, 0x4(%rdx), %xmm2, %xmm11

	pmadcsswd       %xmm11, %xmm3, %xmm2, %xmm11
	pmadcsswd       %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pmadcswd        %xmm11, %xmm3, %xmm2, %xmm11
	pmadcswd        %xmm11, 0x4(%rdx), %xmm2, %xmm11

	pmacssww        %xmm1, %xmm13, %xmm2, %xmm1
	pmacssww        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsww         %xmm1, %xmm13, %xmm2, %xmm1
	pmacsww         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacswd         %xmm1, %xmm13, %xmm2, %xmm1
	pmacswd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacssdd        %xmm1, %xmm13, %xmm2, %xmm1
	pmacssdd        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsdd         %xmm1, %xmm13, %xmm2, %xmm1
	pmacsdd         %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacssdql       %xmm1, %xmm13, %xmm2, %xmm1
	pmacssdql       %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacssdqh       %xmm1, %xmm13, %xmm2, %xmm1
	pmacssdqh       %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsdql        %xmm1, %xmm13, %xmm2, %xmm1
	pmacsdql        %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmacsdqh        %xmm1, %xmm13, %xmm2, %xmm1
	pmacsdqh        %xmm1, 0x4(%rdx), %xmm2, %xmm1

	pmadcsswd       %xmm1, %xmm13, %xmm2, %xmm1
	pmadcsswd       %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pmadcswd        %xmm1, %xmm13, %xmm2, %xmm1
	pmadcswd        %xmm1, 0x4(%rdx), %xmm2, %xmm1

	pmacssww        %xmm1, %xmm3, %xmm2, %xmm1
	pmacssww        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacsww         %xmm1, %xmm3, %xmm2, %xmm1
	pmacsww         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacswd         %xmm1, %xmm3, %xmm2, %xmm1
	pmacswd         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacssdd        %xmm1, %xmm3, %xmm2, %xmm1
	pmacssdd        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacsdd         %xmm1, %xmm3, %xmm2, %xmm1
	pmacsdd         %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacssdql       %xmm1, %xmm3, %xmm2, %xmm1
	pmacssdql       %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacssdqh       %xmm1, %xmm3, %xmm2, %xmm1
	pmacssdqh       %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacsdql        %xmm1, %xmm3, %xmm2, %xmm1
	pmacsdql        %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmacsdqh        %xmm1, %xmm3, %xmm2, %xmm1
	pmacsdqh        %xmm1, 0x100000(%r15), %xmm2, %xmm1

	pmadcsswd       %xmm1, %xmm3, %xmm2, %xmm1
	pmadcsswd       %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pmadcswd        %xmm1, %xmm3, %xmm2, %xmm1
	pmadcswd        %xmm1, 0x100000(%r15), %xmm2, %xmm1

	phaddbw         %xmm2, %xmm1
	phaddbw         0x4(%rdx), %xmm1
	phaddbd         %xmm2, %xmm1
	phaddbd         0x4(%rdx), %xmm1
	phaddbq         %xmm2, %xmm1
	phaddbq         0x4(%rdx), %xmm1
	phaddwd         %xmm2, %xmm1
	phaddwd         0x4(%rdx), %xmm1
	phaddwq         %xmm2, %xmm1
	phaddwq         0x4(%rdx), %xmm1
	phadddq         %xmm2, %xmm1
	phadddq         0x4(%rdx), %xmm1

	phaddubw        %xmm2, %xmm1
	phaddubw        0x4(%rdx), %xmm1
	phaddubd        %xmm2, %xmm1
	phaddubd        0x4(%rdx), %xmm1
	phaddubq        %xmm2, %xmm1
	phaddubq        0x4(%rdx), %xmm1
	phadduwd        %xmm2, %xmm1
	phadduwd        0x4(%rdx), %xmm1
	phadduwq        %xmm2, %xmm1
	phadduwq        0x4(%rdx), %xmm1
	phaddudq        %xmm2, %xmm1
	phaddudq        0x4(%rdx), %xmm1

	phsubbw         %xmm2, %xmm1
	phsubbw         0x4(%rdx), %xmm1
	phsubwd         %xmm2, %xmm1
	phsubwd         0x4(%rdx), %xmm1
	phsubdq         %xmm2, %xmm1
	phsubdq         0x4(%rdx), %xmm1

	phaddbw         %xmm12, %xmm11
	phaddbw         0x100000(%r15), %xmm11
	phaddbd         %xmm12, %xmm11
	phaddbd         0x100000(%r15), %xmm11
	phaddbq         %xmm12, %xmm11
	phaddbq         0x100000(%r15), %xmm11
	phaddwd         %xmm12, %xmm11
	phaddwd         0x100000(%r15), %xmm11
	phaddwq         %xmm12, %xmm11
	phaddwq         0x100000(%r15), %xmm11
	phadddq         %xmm12, %xmm11
	phadddq         0x100000(%r15), %xmm11

	phaddubw        %xmm12, %xmm11
	phaddubw        0x100000(%r15), %xmm11
	phaddubd        %xmm12, %xmm11
	phaddubd        0x100000(%r15), %xmm11
	phaddubq        %xmm12, %xmm11
	phaddubq        0x100000(%r15), %xmm11
	phadduwd        %xmm12, %xmm11
	phadduwd        0x100000(%r15), %xmm11
	phadduwq        %xmm12, %xmm11
	phadduwq        0x100000(%r15), %xmm11
	phaddudq        %xmm12, %xmm11
	phaddudq        0x100000(%r15), %xmm11

	phsubbw         %xmm12, %xmm11
	phsubbw         0x100000(%r15), %xmm11
	phsubwd         %xmm12, %xmm11
	phsubwd         0x100000(%r15), %xmm11
	phsubdq         %xmm12, %xmm11
	phsubdq         0x100000(%r15), %xmm11

	phaddbw         %xmm12, %xmm1
	phaddbw         0x4(%rdx), %xmm1
	phaddbd         %xmm12, %xmm1
	phaddbd         0x4(%rdx), %xmm1
	phaddbq         %xmm12, %xmm1
	phaddbq         0x4(%rdx), %xmm1
	phaddwd         %xmm12, %xmm1
	phaddwd         0x4(%rdx), %xmm1
	phaddwq         %xmm12, %xmm1
	phaddwq         0x4(%rdx), %xmm1
	phadddq         %xmm12, %xmm1
	phadddq         0x4(%rdx), %xmm1

	phaddubw        %xmm12, %xmm1
	phaddubw        0x4(%rdx), %xmm1
	phaddubd        %xmm12, %xmm1
	phaddubd        0x4(%rdx), %xmm1
	phaddubq        %xmm12, %xmm1
	phaddubq        0x4(%rdx), %xmm1
	phadduwd        %xmm12, %xmm1
	phadduwd        0x4(%rdx), %xmm1
	phadduwq        %xmm12, %xmm1
	phadduwq        0x4(%rdx), %xmm1
	phaddudq        %xmm12, %xmm1
	phaddudq        0x4(%rdx), %xmm1

	phsubbw         %xmm12, %xmm1
	phsubbw         0x4(%rdx), %xmm1
	phsubwd         %xmm12, %xmm1
	phsubwd         0x4(%rdx), %xmm1
	phsubdq         %xmm12, %xmm1
	phsubdq         0x4(%rdx), %xmm1

	phaddbw         %xmm2, %xmm11
	phaddbw         0x4(%rdx), %xmm11
	phaddbd         %xmm2, %xmm11
	phaddbd         0x4(%rdx), %xmm11
	phaddbq         %xmm2, %xmm11
	phaddbq         0x4(%rdx), %xmm11
	phaddwd         %xmm2, %xmm11
	phaddwd         0x4(%rdx), %xmm11
	phaddwq         %xmm2, %xmm11
	phaddwq         0x4(%rdx), %xmm11
	phadddq         %xmm2, %xmm11
	phadddq         0x4(%rdx), %xmm11

	phaddubw        %xmm2, %xmm11
	phaddubw        0x4(%rdx), %xmm11
	phaddubd        %xmm2, %xmm11
	phaddubd        0x4(%rdx), %xmm11
	phaddubq        %xmm2, %xmm11
	phaddubq        0x4(%rdx), %xmm11
	phadduwd        %xmm2, %xmm11
	phadduwd        0x4(%rdx), %xmm11
	phadduwq        %xmm2, %xmm11
	phadduwq        0x4(%rdx), %xmm11
	phaddudq        %xmm2, %xmm11
	phaddudq        0x4(%rdx), %xmm11

	phsubbw         %xmm2, %xmm11
	phsubbw         0x4(%rdx), %xmm11
	phsubwd         %xmm2, %xmm11
	phsubwd         0x4(%rdx), %xmm11
	phsubdq         %xmm2, %xmm11
	phsubdq         0x4(%rdx), %xmm11

	phaddbw         %xmm2, %xmm1
	phaddbw         0x4(%rdx), %xmm1
	phaddbd         %xmm2, %xmm1
	phaddbd         0x4(%rdx), %xmm1
	phaddbq         %xmm2, %xmm1
	phaddbq         0x4(%rdx), %xmm1
	phaddwd         %xmm2, %xmm1
	phaddwd         0x4(%rdx), %xmm1
	phaddwq         %xmm2, %xmm1
	phaddwq         0x4(%rdx), %xmm1
	phadddq         %xmm2, %xmm1
	phadddq         0x4(%rdx), %xmm1

	phaddubw        %xmm2, %xmm1
	phaddubw        0x4(%rdx), %xmm1
	phaddubd        %xmm2, %xmm1
	phaddubd        0x4(%rdx), %xmm1
	phaddubq        %xmm2, %xmm1
	phaddubq        0x4(%rdx), %xmm1
	phadduwd        %xmm2, %xmm1
	phadduwd        0x4(%rdx), %xmm1
	phadduwq        %xmm2, %xmm1
	phadduwq        0x4(%rdx), %xmm1
	phaddudq        %xmm2, %xmm1
	phaddudq        0x4(%rdx), %xmm1

	phsubbw         %xmm2, %xmm1
	phsubbw         0x4(%rdx), %xmm1
	phsubwd         %xmm2, %xmm1
	phsubwd         0x4(%rdx), %xmm1
	phsubdq         %xmm2, %xmm1
	phsubdq         0x4(%rdx), %xmm1

	phaddbw         %xmm2, %xmm1
	phaddbw         0x100000(%r15), %xmm1
	phaddbd         %xmm2, %xmm1
	phaddbd         0x100000(%r15), %xmm1
	phaddbq         %xmm2, %xmm1
	phaddbq         0x100000(%r15), %xmm1
	phaddwd         %xmm2, %xmm1
	phaddwd         0x100000(%r15), %xmm1
	phaddwq         %xmm2, %xmm1
	phaddwq         0x100000(%r15), %xmm1
	phadddq         %xmm2, %xmm1
	phadddq         0x100000(%r15), %xmm1

	phaddubw        %xmm2, %xmm1
	phaddubw        0x100000(%r15), %xmm1
	phaddubd        %xmm2, %xmm1
	phaddubd        0x100000(%r15), %xmm1
	phaddubq        %xmm2, %xmm1
	phaddubq        0x100000(%r15), %xmm1
	phadduwd        %xmm2, %xmm1
	phadduwd        0x100000(%r15), %xmm1
	phadduwq        %xmm2, %xmm1
	phadduwq        0x100000(%r15), %xmm1
	phaddudq        %xmm2, %xmm1
	phaddudq        0x100000(%r15), %xmm1

	phsubbw         %xmm2, %xmm1
	phsubbw         0x100000(%r15), %xmm1
	phsubwd         %xmm2, %xmm1
	phsubwd         0x100000(%r15), %xmm1
	phsubdq         %xmm2, %xmm1
	phsubdq         0x100000(%r15), %xmm1

	pcmov           %xmm3, %xmm2, %xmm1, %xmm1
	pcmov           0x4(%rdx), %xmm2, %xmm1, %xmm1
	pcmov           %xmm2, 0x4(%rdx), %xmm1, %xmm1
	pcmov           %xmm1, %xmm3, %xmm2, %xmm1
	pcmov           %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pcmov           %xmm1, %xmm2, 0x4(%rdx), %xmm1

	pperm           %xmm3, %xmm2, %xmm1, %xmm1
	pperm           0x4(%rdx), %xmm2, %xmm1, %xmm1
	pperm           %xmm2, 0x4(%rdx), %xmm1, %xmm1
	pperm           %xmm1, %xmm3, %xmm2, %xmm1
	pperm           %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pperm           %xmm1, %xmm2, 0x4(%rdx), %xmm1

	permps          %xmm3, %xmm2, %xmm1, %xmm1
	permps          0x4(%rdx), %xmm2, %xmm1, %xmm1
	permps          %xmm2, 0x4(%rdx), %xmm1, %xmm1
	permps          %xmm1, %xmm3, %xmm2, %xmm1
	permps          %xmm1, 0x4(%rdx), %xmm2, %xmm1
	permps          %xmm1, %xmm2, 0x4(%rdx), %xmm1

	permpd          %xmm3, %xmm2, %xmm1, %xmm1
	permpd          0x4(%rdx), %xmm2, %xmm1, %xmm1
	permpd          %xmm2, 0x4(%rdx), %xmm1, %xmm1
	permpd          %xmm1, %xmm3, %xmm2, %xmm1
	permpd          %xmm1, 0x4(%rdx), %xmm2, %xmm1
	permpd          %xmm1, %xmm2, 0x4(%rdx), %xmm1

	pcmov           %xmm13, %xmm12, %xmm11, %xmm11
	pcmov           0x100000(%r15), %xmm12, %xmm11, %xmm11
	pcmov           %xmm12, 0x100000(%r15), %xmm11, %xmm11
	pcmov           %xmm11, %xmm13, %xmm12, %xmm11
	pcmov           %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pcmov           %xmm11, %xmm12, 0x100000(%r15), %xmm11

	pperm           %xmm13, %xmm12, %xmm11, %xmm11
	pperm           0x100000(%r15), %xmm12, %xmm11, %xmm11
	pperm           %xmm12, 0x100000(%r15), %xmm11, %xmm11
	pperm           %xmm11, %xmm13, %xmm12, %xmm11
	pperm           %xmm11, 0x100000(%r15), %xmm12, %xmm11
	pperm           %xmm11, %xmm12, 0x100000(%r15), %xmm11

	permps          %xmm13, %xmm12, %xmm11, %xmm11
	permps          0x100000(%r15), %xmm12, %xmm11, %xmm11
	permps          %xmm12, 0x100000(%r15), %xmm11, %xmm11
	permps          %xmm11, %xmm13, %xmm12, %xmm11
	permps          %xmm11, 0x100000(%r15), %xmm12, %xmm11
	permps          %xmm11, %xmm12, 0x100000(%r15), %xmm11

	permpd          %xmm13, %xmm12, %xmm11, %xmm11
	permpd          0x100000(%r15), %xmm12, %xmm11, %xmm11
	permpd          %xmm12, 0x100000(%r15), %xmm11, %xmm11
	permpd          %xmm11, %xmm13, %xmm12, %xmm11
	permpd          %xmm11, 0x100000(%r15), %xmm12, %xmm11
	permpd          %xmm11, %xmm12, 0x100000(%r15), %xmm11

	pcmov           %xmm3, %xmm12, %xmm1, %xmm1
	pcmov           0x4(%rdx), %xmm12, %xmm1, %xmm1
	pcmov           %xmm12, 0x4(%rdx), %xmm1, %xmm1
	pcmov           %xmm1, %xmm3, %xmm12, %xmm1
	pcmov           %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pcmov           %xmm1, %xmm12, 0x4(%rdx), %xmm1

	pperm           %xmm3, %xmm12, %xmm1, %xmm1
	pperm           0x4(%rdx), %xmm12, %xmm1, %xmm1
	pperm           %xmm12, 0x4(%rdx), %xmm1, %xmm1
	pperm           %xmm1, %xmm3, %xmm12, %xmm1
	pperm           %xmm1, 0x4(%rdx), %xmm12, %xmm1
	pperm           %xmm1, %xmm12, 0x4(%rdx), %xmm1

	permps          %xmm3, %xmm12, %xmm1, %xmm1
	permps          0x4(%rdx), %xmm12, %xmm1, %xmm1
	permps          %xmm12, 0x4(%rdx), %xmm1, %xmm1
	permps          %xmm1, %xmm3, %xmm12, %xmm1
	permps          %xmm1, 0x4(%rdx), %xmm12, %xmm1
	permps          %xmm1, %xmm12, 0x4(%rdx), %xmm1

	permpd          %xmm3, %xmm12, %xmm1, %xmm1
	permpd          0x4(%rdx), %xmm12, %xmm1, %xmm1
	permpd          %xmm12, 0x4(%rdx), %xmm1, %xmm1
	permpd          %xmm1, %xmm3, %xmm12, %xmm1
	permpd          %xmm1, 0x4(%rdx), %xmm12, %xmm1
	permpd          %xmm1, %xmm12, 0x4(%rdx), %xmm1

	pcmov           %xmm3, %xmm2, %xmm11, %xmm11
	pcmov           0x4(%rdx), %xmm2, %xmm11, %xmm11
	pcmov           %xmm2, 0x4(%rdx), %xmm11, %xmm11
	pcmov           %xmm11, %xmm3, %xmm2, %xmm11
	pcmov           %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pcmov           %xmm11, %xmm2, 0x4(%rdx), %xmm11

	pperm           %xmm3, %xmm2, %xmm11, %xmm11
	pperm           0x4(%rdx), %xmm2, %xmm11, %xmm11
	pperm           %xmm2, 0x4(%rdx), %xmm11, %xmm11
	pperm           %xmm11, %xmm3, %xmm2, %xmm11
	pperm           %xmm11, 0x4(%rdx), %xmm2, %xmm11
	pperm           %xmm11, %xmm2, 0x4(%rdx), %xmm11

	permps          %xmm3, %xmm2, %xmm11, %xmm11
	permps          0x4(%rdx), %xmm2, %xmm11, %xmm11
	permps          %xmm2, 0x4(%rdx), %xmm11, %xmm11
	permps          %xmm11, %xmm3, %xmm2, %xmm11
	permps          %xmm11, 0x4(%rdx), %xmm2, %xmm11
	permps          %xmm11, %xmm2, 0x4(%rdx), %xmm11

	permpd          %xmm3, %xmm2, %xmm11, %xmm11
	permpd          0x4(%rdx), %xmm2, %xmm11, %xmm11
	permpd          %xmm2, 0x4(%rdx), %xmm11, %xmm11
	permpd          %xmm11, %xmm3, %xmm2, %xmm11
	permpd          %xmm11, 0x4(%rdx), %xmm2, %xmm11
	permpd          %xmm11, %xmm2, 0x4(%rdx), %xmm11

	pcmov           %xmm13, %xmm2, %xmm1, %xmm1
	pcmov           0x4(%rdx), %xmm2, %xmm1, %xmm1
	pcmov           %xmm2, 0x4(%rdx), %xmm1, %xmm1
	pcmov           %xmm1, %xmm13, %xmm2, %xmm1
	pcmov           %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pcmov           %xmm1, %xmm2, 0x4(%rdx), %xmm1

	pperm           %xmm13, %xmm2, %xmm1, %xmm1
	pperm           0x4(%rdx), %xmm2, %xmm1, %xmm1
	pperm           %xmm2, 0x4(%rdx), %xmm1, %xmm1
	pperm           %xmm1, %xmm13, %xmm2, %xmm1
	pperm           %xmm1, 0x4(%rdx), %xmm2, %xmm1
	pperm           %xmm1, %xmm2, 0x4(%rdx), %xmm1

	permps          %xmm13, %xmm2, %xmm1, %xmm1
	permps          0x4(%rdx), %xmm2, %xmm1, %xmm1
	permps          %xmm2, 0x4(%rdx), %xmm1, %xmm1
	permps          %xmm1, %xmm13, %xmm2, %xmm1
	permps          %xmm1, 0x4(%rdx), %xmm2, %xmm1
	permps          %xmm1, %xmm2, 0x4(%rdx), %xmm1

	permpd          %xmm13, %xmm2, %xmm1, %xmm1
	permpd          0x4(%rdx), %xmm2, %xmm1, %xmm1
	permpd          %xmm2, 0x4(%rdx), %xmm1, %xmm1
	permpd          %xmm1, %xmm13, %xmm2, %xmm1
	permpd          %xmm1, 0x4(%rdx), %xmm2, %xmm1
	permpd          %xmm1, %xmm2, 0x4(%rdx), %xmm1

	pcmov           %xmm3, %xmm2, %xmm1, %xmm1
	pcmov           0x100000(%r15), %xmm2, %xmm1, %xmm1
	pcmov           %xmm2, 0x100000(%r15), %xmm1, %xmm1
	pcmov           %xmm1, %xmm3, %xmm2, %xmm1
	pcmov           %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pcmov           %xmm1, %xmm2, 0x100000(%r15), %xmm1

	pperm           %xmm3, %xmm2, %xmm1, %xmm1
	pperm           0x100000(%r15), %xmm2, %xmm1, %xmm1
	pperm           %xmm2, 0x100000(%r15), %xmm1, %xmm1
	pperm           %xmm1, %xmm3, %xmm2, %xmm1
	pperm           %xmm1, 0x100000(%r15), %xmm2, %xmm1
	pperm           %xmm1, %xmm2, 0x100000(%r15), %xmm1

	permps          %xmm3, %xmm2, %xmm1, %xmm1
	permps          0x100000(%r15), %xmm2, %xmm1, %xmm1
	permps          %xmm2, 0x100000(%r15), %xmm1, %xmm1
	permps          %xmm1, %xmm3, %xmm2, %xmm1
	permps          %xmm1, 0x100000(%r15), %xmm2, %xmm1
	permps          %xmm1, %xmm2, 0x100000(%r15), %xmm1

	permpd          %xmm3, %xmm2, %xmm1, %xmm1
	permpd          0x100000(%r15), %xmm2, %xmm1, %xmm1
	permpd          %xmm2, 0x100000(%r15), %xmm1, %xmm1
	permpd          %xmm1, %xmm3, %xmm2, %xmm1
	permpd          %xmm1, 0x100000(%r15), %xmm2, %xmm1
	permpd          %xmm1, %xmm2, 0x100000(%r15), %xmm1

	protb           %xmm3, %xmm2, %xmm1
	protb           0x4(%rdx), %xmm2, %xmm1
	protb           %xmm2, 0x4(%rdx), %xmm1
	protb           $0x4, %xmm2, %xmm1
	protw           %xmm3, %xmm2, %xmm1
	protw           0x4(%rdx), %xmm2, %xmm1
	protw           %xmm2, 0x4(%rdx), %xmm1
	protw           $0x4, %xmm2, %xmm1
	protd           %xmm3, %xmm2, %xmm1
	protd           0x4(%rdx), %xmm2, %xmm1
	protd           %xmm2, 0x4(%rdx), %xmm1
	protd           $0x4, %xmm2, %xmm1
	protq           %xmm3, %xmm2, %xmm1
	protq           0x4(%rdx), %xmm2, %xmm1
	protq           %xmm2, 0x4(%rdx), %xmm1
	protq           $0x4, %xmm2, %xmm1

	protb           %xmm13, %xmm12, %xmm11
	protb           0x100000(%r15), %xmm12, %xmm11
	protb           %xmm12, 0x100000(%r15), %xmm11
	protb           $0x4, %xmm12, %xmm11
	protw           %xmm13, %xmm12, %xmm11
	protw           0x100000(%r15), %xmm12, %xmm11
	protw           %xmm12, 0x100000(%r15), %xmm11
	protw           $0x4, %xmm12, %xmm11
	protd           %xmm13, %xmm12, %xmm11
	protd           0x100000(%r15), %xmm12, %xmm11
	protd           %xmm12, 0x100000(%r15), %xmm11
	protd           $0x4, %xmm12, %xmm11
	protq           %xmm13, %xmm12, %xmm11
	protq           0x100000(%r15), %xmm12, %xmm11
	protq           %xmm12, 0x100000(%r15), %xmm11
	protq           $0x4, %xmm12, %xmm11

	protb           %xmm3, %xmm12, %xmm1
	protb           0x4(%rdx), %xmm12, %xmm1
	protb           %xmm12, 0x4(%rdx), %xmm1
	protb           $0x4, %xmm12, %xmm1
	protw           %xmm3, %xmm12, %xmm1
	protw           0x4(%rdx), %xmm12, %xmm1
	protw           %xmm12, 0x4(%rdx), %xmm1
	protw           $0x4, %xmm12, %xmm1
	protd           %xmm3, %xmm12, %xmm1
	protd           0x4(%rdx), %xmm12, %xmm1
	protd           %xmm12, 0x4(%rdx), %xmm1
	protd           $0x4, %xmm12, %xmm1
	protq           %xmm3, %xmm12, %xmm1
	protq           0x4(%rdx), %xmm12, %xmm1
	protq           %xmm12, 0x4(%rdx), %xmm1
	protq           $0x4, %xmm12, %xmm1

	protb           %xmm3, %xmm2, %xmm11
	protb           0x4(%rdx), %xmm2, %xmm11
	protb           %xmm2, 0x4(%rdx), %xmm11
	protb           $0x4, %xmm2, %xmm11
	protw           %xmm3, %xmm2, %xmm11
	protw           0x4(%rdx), %xmm2, %xmm11
	protw           %xmm2, 0x4(%rdx), %xmm11
	protw           $0x4, %xmm2, %xmm11
	protd           %xmm3, %xmm2, %xmm11
	protd           0x4(%rdx), %xmm2, %xmm11
	protd           %xmm2, 0x4(%rdx), %xmm11
	protd           $0x4, %xmm2, %xmm11
	protq           %xmm3, %xmm2, %xmm11
	protq           0x4(%rdx), %xmm2, %xmm11
	protq           %xmm2, 0x4(%rdx), %xmm11
	protq           $0x4, %xmm2, %xmm11

	protb           %xmm13, %xmm2, %xmm1
	protb           0x4(%rdx), %xmm2, %xmm1
	protb           %xmm2, 0x4(%rdx), %xmm1
	protb           $0x4, %xmm2, %xmm1
	protw           %xmm13, %xmm2, %xmm1
	protw           0x4(%rdx), %xmm2, %xmm1
	protw           %xmm2, 0x4(%rdx), %xmm1
	protw           $0x4, %xmm2, %xmm1
	protd           %xmm13, %xmm2, %xmm1
	protd           0x4(%rdx), %xmm2, %xmm1
	protd           %xmm2, 0x4(%rdx), %xmm1
	protd           $0x4, %xmm2, %xmm1
	protq           %xmm13, %xmm2, %xmm1
	protq           0x4(%rdx), %xmm2, %xmm1
	protq           %xmm2, 0x4(%rdx), %xmm1
	protq           $0x4, %xmm2, %xmm1

	protb           %xmm3, %xmm2, %xmm1
	protb           0x100000(%r15), %xmm2, %xmm1
	protb           %xmm2, 0x100000(%r15), %xmm1
	protb           $0x4, %xmm2, %xmm1
	protw           %xmm3, %xmm2, %xmm1
	protw           0x100000(%r15), %xmm2, %xmm1
	protw           %xmm2, 0x100000(%r15), %xmm1
	protw           $0x4, %xmm2, %xmm1
	protd           %xmm3, %xmm2, %xmm1
	protd           0x100000(%r15), %xmm2, %xmm1
	protd           %xmm2, 0x100000(%r15), %xmm1
	protd           $0x4, %xmm2, %xmm1
	protq           %xmm3, %xmm2, %xmm1
	protq           0x100000(%r15), %xmm2, %xmm1
	protq           %xmm2, 0x100000(%r15), %xmm1
	protq           $0x4, %xmm2, %xmm1

	pshlb           %xmm3, %xmm2, %xmm1
	pshlb           0x4(%rdx), %xmm2, %xmm1
	pshlb           %xmm2, 0x4(%rdx), %xmm1
	pshlw           %xmm3, %xmm2, %xmm1
	pshlw           0x4(%rdx), %xmm2, %xmm1
	pshlw           %xmm2, 0x4(%rdx), %xmm1
	pshld           %xmm3, %xmm2, %xmm1
	pshld           0x4(%rdx), %xmm2, %xmm1
	pshld           %xmm2, 0x4(%rdx), %xmm1
	pshlq           %xmm3, %xmm2, %xmm1
	pshlq           0x4(%rdx), %xmm2, %xmm1
	pshlq           %xmm2, 0x4(%rdx), %xmm1

	pshab           %xmm3, %xmm2, %xmm1
	pshab           0x4(%rdx), %xmm2, %xmm1
	pshab           %xmm2, 0x4(%rdx), %xmm1
	pshaw           %xmm3, %xmm2, %xmm1
	pshaw           0x4(%rdx), %xmm2, %xmm1
	pshaw           %xmm2, 0x4(%rdx), %xmm1
	pshad           %xmm3, %xmm2, %xmm1
	pshad           0x4(%rdx), %xmm2, %xmm1
	pshad           %xmm2, 0x4(%rdx), %xmm1
	pshaq           %xmm3, %xmm2, %xmm1
	pshaq           0x4(%rdx), %xmm2, %xmm1
	pshaq           %xmm2, 0x4(%rdx), %xmm1

	pshlb           %xmm13, %xmm12, %xmm11
	pshlb           0x100000(%r15), %xmm12, %xmm11
	pshlb           %xmm12, 0x100000(%r15), %xmm11
	pshlw           %xmm13, %xmm12, %xmm11
	pshlw           0x100000(%r15), %xmm12, %xmm11
	pshlw           %xmm12, 0x100000(%r15), %xmm11
	pshld           %xmm13, %xmm12, %xmm11
	pshld           0x100000(%r15), %xmm12, %xmm11
	pshld           %xmm12, 0x100000(%r15), %xmm11
	pshlq           %xmm13, %xmm12, %xmm11
	pshlq           0x100000(%r15), %xmm12, %xmm11
	pshlq           %xmm12, 0x100000(%r15), %xmm11

	pshab           %xmm13, %xmm12, %xmm11
	pshab           0x100000(%r15), %xmm12, %xmm11
	pshab           %xmm12, 0x100000(%r15), %xmm11
	pshaw           %xmm13, %xmm12, %xmm11
	pshaw           0x100000(%r15), %xmm12, %xmm11
	pshaw           %xmm12, 0x100000(%r15), %xmm11
	pshad           %xmm13, %xmm12, %xmm11
	pshad           0x100000(%r15), %xmm12, %xmm11
	pshad           %xmm12, 0x100000(%r15), %xmm11
	pshaq           %xmm13, %xmm12, %xmm11
	pshaq           0x100000(%r15), %xmm12, %xmm11
	pshaq           %xmm12, 0x100000(%r15), %xmm11

	pshlb           %xmm3, %xmm12, %xmm1
	pshlb           0x4(%rdx), %xmm12, %xmm1
	pshlb           %xmm12, 0x4(%rdx), %xmm1
	pshlw           %xmm3, %xmm12, %xmm1
	pshlw           0x4(%rdx), %xmm12, %xmm1
	pshlw           %xmm12, 0x4(%rdx), %xmm1
	pshld           %xmm3, %xmm12, %xmm1
	pshld           0x4(%rdx), %xmm12, %xmm1
	pshld           %xmm12, 0x4(%rdx), %xmm1
	pshlq           %xmm3, %xmm12, %xmm1
	pshlq           0x4(%rdx), %xmm12, %xmm1
	pshlq           %xmm12, 0x4(%rdx), %xmm1

	pshab           %xmm3, %xmm12, %xmm1
	pshab           0x4(%rdx), %xmm12, %xmm1
	pshab           %xmm12, 0x4(%rdx), %xmm1
	pshaw           %xmm3, %xmm12, %xmm1
	pshaw           0x4(%rdx), %xmm12, %xmm1
	pshaw           %xmm12, 0x4(%rdx), %xmm1
	pshad           %xmm3, %xmm12, %xmm1
	pshad           0x4(%rdx), %xmm12, %xmm1
	pshad           %xmm12, 0x4(%rdx), %xmm1
	pshaq           %xmm3, %xmm12, %xmm1
	pshaq           0x4(%rdx), %xmm12, %xmm1
	pshaq           %xmm12, 0x4(%rdx), %xmm1

	pshlb           %xmm3, %xmm2, %xmm11
	pshlb           0x4(%rdx), %xmm2, %xmm11
	pshlb           %xmm2, 0x4(%rdx), %xmm11
	pshlw           %xmm3, %xmm2, %xmm11
	pshlw           0x4(%rdx), %xmm2, %xmm11
	pshlw           %xmm2, 0x4(%rdx), %xmm11
	pshld           %xmm3, %xmm2, %xmm11
	pshld           0x4(%rdx), %xmm2, %xmm11
	pshld           %xmm2, 0x4(%rdx), %xmm11
	pshlq           %xmm3, %xmm2, %xmm11
	pshlq           0x4(%rdx), %xmm2, %xmm11
	pshlq           %xmm2, 0x4(%rdx), %xmm11

	pshab           %xmm3, %xmm2, %xmm11
	pshab           0x4(%rdx), %xmm2, %xmm11
	pshab           %xmm2, 0x4(%rdx), %xmm11
	pshaw           %xmm3, %xmm2, %xmm11
	pshaw           0x4(%rdx), %xmm2, %xmm11
	pshaw           %xmm2, 0x4(%rdx), %xmm11
	pshad           %xmm3, %xmm2, %xmm11
	pshad           0x4(%rdx), %xmm2, %xmm11
	pshad           %xmm2, 0x4(%rdx), %xmm11
	pshaq           %xmm3, %xmm2, %xmm11
	pshaq           0x4(%rdx), %xmm2, %xmm11
	pshaq           %xmm2, 0x4(%rdx), %xmm11

	pshlb           %xmm13, %xmm2, %xmm1
	pshlb           0x4(%rdx), %xmm2, %xmm1
	pshlb           %xmm2, 0x4(%rdx), %xmm1
	pshlw           %xmm13, %xmm2, %xmm1
	pshlw           0x4(%rdx), %xmm2, %xmm1
	pshlw           %xmm2, 0x4(%rdx), %xmm1
	pshld           %xmm13, %xmm2, %xmm1
	pshld           0x4(%rdx), %xmm2, %xmm1
	pshld           %xmm2, 0x4(%rdx), %xmm1
	pshlq           %xmm13, %xmm2, %xmm1
	pshlq           0x4(%rdx), %xmm2, %xmm1
	pshlq           %xmm2, 0x4(%rdx), %xmm1

	pshab           %xmm13, %xmm2, %xmm1
	pshab           0x4(%rdx), %xmm2, %xmm1
	pshab           %xmm2, 0x4(%rdx), %xmm1
	pshaw           %xmm13, %xmm2, %xmm1
	pshaw           0x4(%rdx), %xmm2, %xmm1
	pshaw           %xmm2, 0x4(%rdx), %xmm1
	pshad           %xmm13, %xmm2, %xmm1
	pshad           0x4(%rdx), %xmm2, %xmm1
	pshad           %xmm2, 0x4(%rdx), %xmm1
	pshaq           %xmm13, %xmm2, %xmm1
	pshaq           0x4(%rdx), %xmm2, %xmm1
	pshaq           %xmm2, 0x4(%rdx), %xmm1

	pshlb           %xmm3, %xmm2, %xmm1
	pshlb           0x100000(%r15), %xmm2, %xmm1
	pshlb           %xmm2, 0x100000(%r15), %xmm1
	pshlw           %xmm3, %xmm2, %xmm1
	pshlw           0x100000(%r15), %xmm2, %xmm1
	pshlw           %xmm2, 0x100000(%r15), %xmm1
	pshld           %xmm3, %xmm2, %xmm1
	pshld           0x100000(%r15), %xmm2, %xmm1
	pshld           %xmm2, 0x100000(%r15), %xmm1
	pshlq           %xmm3, %xmm2, %xmm1
	pshlq           0x100000(%r15), %xmm2, %xmm1
	pshlq           %xmm2, 0x100000(%r15), %xmm1

	pshab           %xmm3, %xmm2, %xmm1
	pshab           0x100000(%r15), %xmm2, %xmm1
	pshab           %xmm2, 0x100000(%r15), %xmm1
	pshaw           %xmm3, %xmm2, %xmm1
	pshaw           0x100000(%r15), %xmm2, %xmm1
	pshaw           %xmm2, 0x100000(%r15), %xmm1
	pshad           %xmm3, %xmm2, %xmm1
	pshad           0x100000(%r15), %xmm2, %xmm1
	pshad           %xmm2, 0x100000(%r15), %xmm1
	pshaq           %xmm3, %xmm2, %xmm1
	pshaq           0x100000(%r15), %xmm2, %xmm1
	pshaq           %xmm2, 0x100000(%r15), %xmm1

	comss           $0x4, %xmm3, %xmm2, %xmm1
	comss           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqss         %xmm3, %xmm2, %xmm1
	comltss         %xmm3, %xmm2, %xmm1
	comless         %xmm3, %xmm2, %xmm1
	comunordss      %xmm3, %xmm2, %xmm1
	comness         %xmm3, %xmm2, %xmm1
	comnltss        %xmm3, %xmm2, %xmm1
	comnless        %xmm3, %xmm2, %xmm1
	comordss        %xmm3, %xmm2, %xmm1
	comueqss        %xmm3, %xmm2, %xmm1
	comultss        %xmm3, %xmm2, %xmm1
	comuless        %xmm3, %xmm2, %xmm1
	comfalsess      %xmm3, %xmm2, %xmm1
	comuness        %xmm3, %xmm2, %xmm1
	comunltss       %xmm3, %xmm2, %xmm1
	comunless       %xmm3, %xmm2, %xmm1
	comtruess       %xmm3, %xmm2, %xmm1
	comeqss         0x4(%rdx), %xmm2, %xmm1
	comltss         0x4(%rdx), %xmm2, %xmm1
	comless         0x4(%rdx), %xmm2, %xmm1
	comunordss      0x4(%rdx), %xmm2, %xmm1
	comness         0x4(%rdx), %xmm2, %xmm1
	comnltss        0x4(%rdx), %xmm2, %xmm1
	comnless        0x4(%rdx), %xmm2, %xmm1
	comordss        0x4(%rdx), %xmm2, %xmm1
	comueqss        0x4(%rdx), %xmm2, %xmm1
	comultss        0x4(%rdx), %xmm2, %xmm1
	comuless        0x4(%rdx), %xmm2, %xmm1
	comfalsess      0x4(%rdx), %xmm2, %xmm1
	comuness        0x4(%rdx), %xmm2, %xmm1
	comunltss       0x4(%rdx), %xmm2, %xmm1
	comunless       0x4(%rdx), %xmm2, %xmm1
	comtruess       0x4(%rdx), %xmm2, %xmm1
	comsd           $0x4, %xmm3, %xmm2, %xmm1
	comsd           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqsd         %xmm3, %xmm2, %xmm1
	comltsd         %xmm3, %xmm2, %xmm1
	comlesd         %xmm3, %xmm2, %xmm1
	comunordsd      %xmm3, %xmm2, %xmm1
	comnesd         %xmm3, %xmm2, %xmm1
	comnltsd        %xmm3, %xmm2, %xmm1
	comnlesd        %xmm3, %xmm2, %xmm1
	comordsd        %xmm3, %xmm2, %xmm1
	comueqsd        %xmm3, %xmm2, %xmm1
	comultsd        %xmm3, %xmm2, %xmm1
	comulesd        %xmm3, %xmm2, %xmm1
	comfalsesd      %xmm3, %xmm2, %xmm1
	comunesd        %xmm3, %xmm2, %xmm1
	comunltsd       %xmm3, %xmm2, %xmm1
	comunlesd       %xmm3, %xmm2, %xmm1
	comtruesd       %xmm3, %xmm2, %xmm1
	comeqsd         0x4(%rdx), %xmm2, %xmm1
	comltsd         0x4(%rdx), %xmm2, %xmm1
	comlesd         0x4(%rdx), %xmm2, %xmm1
	comunordsd      0x4(%rdx), %xmm2, %xmm1
	comnesd         0x4(%rdx), %xmm2, %xmm1
	comnltsd        0x4(%rdx), %xmm2, %xmm1
	comnlesd        0x4(%rdx), %xmm2, %xmm1
	comordsd        0x4(%rdx), %xmm2, %xmm1
	comueqsd        0x4(%rdx), %xmm2, %xmm1
	comultsd        0x4(%rdx), %xmm2, %xmm1
	comulesd        0x4(%rdx), %xmm2, %xmm1
	comfalsesd      0x4(%rdx), %xmm2, %xmm1
	comunesd        0x4(%rdx), %xmm2, %xmm1
	comunltsd       0x4(%rdx), %xmm2, %xmm1
	comunlesd       0x4(%rdx), %xmm2, %xmm1
	comtruesd       0x4(%rdx), %xmm2, %xmm1
	comps           $0x4, %xmm3, %xmm2, %xmm1
	comps           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqps         %xmm3, %xmm2, %xmm1
	comltps         %xmm3, %xmm2, %xmm1
	comleps         %xmm3, %xmm2, %xmm1
	comunordps      %xmm3, %xmm2, %xmm1
	comneps         %xmm3, %xmm2, %xmm1
	comnltps        %xmm3, %xmm2, %xmm1
	comnleps        %xmm3, %xmm2, %xmm1
	comordps        %xmm3, %xmm2, %xmm1
	comueqps        %xmm3, %xmm2, %xmm1
	comultps        %xmm3, %xmm2, %xmm1
	comuleps        %xmm3, %xmm2, %xmm1
	comfalseps      %xmm3, %xmm2, %xmm1
	comuneps        %xmm3, %xmm2, %xmm1
	comunltps       %xmm3, %xmm2, %xmm1
	comunleps       %xmm3, %xmm2, %xmm1
	comtrueps       %xmm3, %xmm2, %xmm1
	comeqps         0x4(%rdx), %xmm2, %xmm1
	comltps         0x4(%rdx), %xmm2, %xmm1
	comleps         0x4(%rdx), %xmm2, %xmm1
	comunordps      0x4(%rdx), %xmm2, %xmm1
	comneps         0x4(%rdx), %xmm2, %xmm1
	comnltps        0x4(%rdx), %xmm2, %xmm1
	comnleps        0x4(%rdx), %xmm2, %xmm1
	comordps        0x4(%rdx), %xmm2, %xmm1
	comueqps        0x4(%rdx), %xmm2, %xmm1
	comultps        0x4(%rdx), %xmm2, %xmm1
	comuleps        0x4(%rdx), %xmm2, %xmm1
	comfalseps      0x4(%rdx), %xmm2, %xmm1
	comuneps        0x4(%rdx), %xmm2, %xmm1
	comunltps       0x4(%rdx), %xmm2, %xmm1
	comunleps       0x4(%rdx), %xmm2, %xmm1
	comtrueps       0x4(%rdx), %xmm2, %xmm1
	compd           $0x4, %xmm3, %xmm2, %xmm1
	compd           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqpd         %xmm3, %xmm2, %xmm1
	comltpd         %xmm3, %xmm2, %xmm1
	comlepd         %xmm3, %xmm2, %xmm1
	comunordpd      %xmm3, %xmm2, %xmm1
	comnepd         %xmm3, %xmm2, %xmm1
	comnltpd        %xmm3, %xmm2, %xmm1
	comnlepd        %xmm3, %xmm2, %xmm1
	comordpd        %xmm3, %xmm2, %xmm1
	comueqpd        %xmm3, %xmm2, %xmm1
	comultpd        %xmm3, %xmm2, %xmm1
	comulepd        %xmm3, %xmm2, %xmm1
	comfalsepd      %xmm3, %xmm2, %xmm1
	comunepd        %xmm3, %xmm2, %xmm1
	comunltpd       %xmm3, %xmm2, %xmm1
	comunlepd       %xmm3, %xmm2, %xmm1
	comtruepd       %xmm3, %xmm2, %xmm1
	comeqpd         0x4(%rdx), %xmm2, %xmm1
	comltpd         0x4(%rdx), %xmm2, %xmm1
	comlepd         0x4(%rdx), %xmm2, %xmm1
	comunordpd      0x4(%rdx), %xmm2, %xmm1
	comnepd         0x4(%rdx), %xmm2, %xmm1
	comnltpd        0x4(%rdx), %xmm2, %xmm1
	comnlepd        0x4(%rdx), %xmm2, %xmm1
	comordpd        0x4(%rdx), %xmm2, %xmm1
	comueqpd        0x4(%rdx), %xmm2, %xmm1
	comultpd        0x4(%rdx), %xmm2, %xmm1
	comulepd        0x4(%rdx), %xmm2, %xmm1
	comfalsepd      0x4(%rdx), %xmm2, %xmm1
	comunepd        0x4(%rdx), %xmm2, %xmm1
	comunltpd       0x4(%rdx), %xmm2, %xmm1
	comunlepd       0x4(%rdx), %xmm2, %xmm1
	comtruepd       0x4(%rdx), %xmm2, %xmm1

	pcomb           $0x4, %xmm3, %xmm2, %xmm1
	pcomb           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltb         %xmm3, %xmm2, %xmm1
	pcomleb         %xmm3, %xmm2, %xmm1
	pcomgtb         %xmm3, %xmm2, %xmm1
	pcomgeb         %xmm3, %xmm2, %xmm1
	pcomeqb         %xmm3, %xmm2, %xmm1
	pcomneqb        %xmm3, %xmm2, %xmm1
	pcomltb         0x4(%rdx), %xmm2, %xmm1
	pcomleb         0x4(%rdx), %xmm2, %xmm1
	pcomgtb         0x4(%rdx), %xmm2, %xmm1
	pcomgeb         0x4(%rdx), %xmm2, %xmm1
	pcomeqb         0x4(%rdx), %xmm2, %xmm1
	pcomneqb        0x4(%rdx), %xmm2, %xmm1
	pcomw           $0x4, %xmm3, %xmm2, %xmm1
	pcomw           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltw         %xmm3, %xmm2, %xmm1
	pcomlew         %xmm3, %xmm2, %xmm1
	pcomgtw         %xmm3, %xmm2, %xmm1
	pcomgew         %xmm3, %xmm2, %xmm1
	pcomeqw         %xmm3, %xmm2, %xmm1
	pcomneqw        %xmm3, %xmm2, %xmm1
	pcomltw         0x4(%rdx), %xmm2, %xmm1
	pcomlew         0x4(%rdx), %xmm2, %xmm1
	pcomgtw         0x4(%rdx), %xmm2, %xmm1
	pcomgew         0x4(%rdx), %xmm2, %xmm1
	pcomeqw         0x4(%rdx), %xmm2, %xmm1
	pcomneqw        0x4(%rdx), %xmm2, %xmm1
	pcomd           $0x4, %xmm3, %xmm2, %xmm1
	pcomd           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltd         %xmm3, %xmm2, %xmm1
	pcomled         %xmm3, %xmm2, %xmm1
	pcomgtd         %xmm3, %xmm2, %xmm1
	pcomged         %xmm3, %xmm2, %xmm1
	pcomeqd         %xmm3, %xmm2, %xmm1
	pcomneqd        %xmm3, %xmm2, %xmm1
	pcomltd         0x4(%rdx), %xmm2, %xmm1
	pcomled         0x4(%rdx), %xmm2, %xmm1
	pcomgtd         0x4(%rdx), %xmm2, %xmm1
	pcomged         0x4(%rdx), %xmm2, %xmm1
	pcomeqd         0x4(%rdx), %xmm2, %xmm1
	pcomneqd        0x4(%rdx), %xmm2, %xmm1
	pcomq           $0x4, %xmm3, %xmm2, %xmm1
	pcomq           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltq         %xmm3, %xmm2, %xmm1
	pcomleq         %xmm3, %xmm2, %xmm1
	pcomgtq         %xmm3, %xmm2, %xmm1
	pcomgeq         %xmm3, %xmm2, %xmm1
	pcomeqq         %xmm3, %xmm2, %xmm1
	pcomneqq        %xmm3, %xmm2, %xmm1
	pcomltq         0x4(%rdx), %xmm2, %xmm1
	pcomleq         0x4(%rdx), %xmm2, %xmm1
	pcomgtq         0x4(%rdx), %xmm2, %xmm1
	pcomgeq         0x4(%rdx), %xmm2, %xmm1
	pcomeqq         0x4(%rdx), %xmm2, %xmm1
	pcomneqq        0x4(%rdx), %xmm2, %xmm1

	pcomub          $0x4, %xmm3, %xmm2, %xmm1
	pcomub          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltub        %xmm3, %xmm2, %xmm1
	pcomleub        %xmm3, %xmm2, %xmm1
	pcomgtub        %xmm3, %xmm2, %xmm1
	pcomgeub        %xmm3, %xmm2, %xmm1
	pcomequb        %xmm3, %xmm2, %xmm1
	pcomnequb       %xmm3, %xmm2, %xmm1
	pcomltub        0x4(%rdx), %xmm2, %xmm1
	pcomleub        0x4(%rdx), %xmm2, %xmm1
	pcomgtub        0x4(%rdx), %xmm2, %xmm1
	pcomgeub        0x4(%rdx), %xmm2, %xmm1
	pcomequb        0x4(%rdx), %xmm2, %xmm1
	pcomnequb       0x4(%rdx), %xmm2, %xmm1
	pcomuw          $0x4, %xmm3, %xmm2, %xmm1
	pcomuw          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltuw        %xmm3, %xmm2, %xmm1
	pcomleuw        %xmm3, %xmm2, %xmm1
	pcomgtuw        %xmm3, %xmm2, %xmm1
	pcomgeuw        %xmm3, %xmm2, %xmm1
	pcomequw        %xmm3, %xmm2, %xmm1
	pcomnequw       %xmm3, %xmm2, %xmm1
	pcomltuw        0x4(%rdx), %xmm2, %xmm1
	pcomleuw        0x4(%rdx), %xmm2, %xmm1
	pcomgtuw        0x4(%rdx), %xmm2, %xmm1
	pcomgeuw        0x4(%rdx), %xmm2, %xmm1
	pcomequw        0x4(%rdx), %xmm2, %xmm1
	pcomnequw       0x4(%rdx), %xmm2, %xmm1
	pcomud          $0x4, %xmm3, %xmm2, %xmm1
	pcomud          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltud        %xmm3, %xmm2, %xmm1
	pcomleud        %xmm3, %xmm2, %xmm1
	pcomgtud        %xmm3, %xmm2, %xmm1
	pcomgeud        %xmm3, %xmm2, %xmm1
	pcomequd        %xmm3, %xmm2, %xmm1
	pcomnequd       %xmm3, %xmm2, %xmm1
	pcomltud        0x4(%rdx), %xmm2, %xmm1
	pcomleud        0x4(%rdx), %xmm2, %xmm1
	pcomgtud        0x4(%rdx), %xmm2, %xmm1
	pcomgeud        0x4(%rdx), %xmm2, %xmm1
	pcomequd        0x4(%rdx), %xmm2, %xmm1
	pcomnequd       0x4(%rdx), %xmm2, %xmm1
	pcomuq          $0x4, %xmm3, %xmm2, %xmm1
	pcomuq          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltuq        %xmm3, %xmm2, %xmm1
	pcomleuq        %xmm3, %xmm2, %xmm1
	pcomgtuq        %xmm3, %xmm2, %xmm1
	pcomgeuq        %xmm3, %xmm2, %xmm1
	pcomequq        %xmm3, %xmm2, %xmm1
	pcomnequq       %xmm3, %xmm2, %xmm1
	pcomltuq        0x4(%rdx), %xmm2, %xmm1
	pcomleuq        0x4(%rdx), %xmm2, %xmm1
	pcomgtuq        0x4(%rdx), %xmm2, %xmm1
	pcomgeuq        0x4(%rdx), %xmm2, %xmm1
	pcomequq        0x4(%rdx), %xmm2, %xmm1
	pcomnequq       0x4(%rdx), %xmm2, %xmm1

	comss           $0x4, %xmm13, %xmm12, %xmm11
	comss           $0x4, 0x100000(%r15), %xmm12, %xmm11
	comeqss         %xmm13, %xmm12, %xmm11
	comltss         %xmm13, %xmm12, %xmm11
	comless         %xmm13, %xmm12, %xmm11
	comunordss      %xmm13, %xmm12, %xmm11
	comness         %xmm13, %xmm12, %xmm11
	comnltss        %xmm13, %xmm12, %xmm11
	comnless        %xmm13, %xmm12, %xmm11
	comordss        %xmm13, %xmm12, %xmm11
	comueqss        %xmm13, %xmm12, %xmm11
	comultss        %xmm13, %xmm12, %xmm11
	comuless        %xmm13, %xmm12, %xmm11
	comfalsess      %xmm13, %xmm12, %xmm11
	comuness        %xmm13, %xmm12, %xmm11
	comunltss       %xmm13, %xmm12, %xmm11
	comunless       %xmm13, %xmm12, %xmm11
	comtruess       %xmm13, %xmm12, %xmm11
	comeqss         0x100000(%r15), %xmm12, %xmm11
	comltss         0x100000(%r15), %xmm12, %xmm11
	comless         0x100000(%r15), %xmm12, %xmm11
	comunordss      0x100000(%r15), %xmm12, %xmm11
	comness         0x100000(%r15), %xmm12, %xmm11
	comnltss        0x100000(%r15), %xmm12, %xmm11
	comnless        0x100000(%r15), %xmm12, %xmm11
	comordss        0x100000(%r15), %xmm12, %xmm11
	comueqss        0x100000(%r15), %xmm12, %xmm11
	comultss        0x100000(%r15), %xmm12, %xmm11
	comuless        0x100000(%r15), %xmm12, %xmm11
	comfalsess      0x100000(%r15), %xmm12, %xmm11
	comuness        0x100000(%r15), %xmm12, %xmm11
	comunltss       0x100000(%r15), %xmm12, %xmm11
	comunless       0x100000(%r15), %xmm12, %xmm11
	comtruess       0x100000(%r15), %xmm12, %xmm11
	comsd           $0x4, %xmm13, %xmm12, %xmm11
	comsd           $0x4, 0x100000(%r15), %xmm12, %xmm11
	comeqsd         %xmm13, %xmm12, %xmm11
	comltsd         %xmm13, %xmm12, %xmm11
	comlesd         %xmm13, %xmm12, %xmm11
	comunordsd      %xmm13, %xmm12, %xmm11
	comnesd         %xmm13, %xmm12, %xmm11
	comnltsd        %xmm13, %xmm12, %xmm11
	comnlesd        %xmm13, %xmm12, %xmm11
	comordsd        %xmm13, %xmm12, %xmm11
	comueqsd        %xmm13, %xmm12, %xmm11
	comultsd        %xmm13, %xmm12, %xmm11
	comulesd        %xmm13, %xmm12, %xmm11
	comfalsesd      %xmm13, %xmm12, %xmm11
	comunesd        %xmm13, %xmm12, %xmm11
	comunltsd       %xmm13, %xmm12, %xmm11
	comunlesd       %xmm13, %xmm12, %xmm11
	comtruesd       %xmm13, %xmm12, %xmm11
	comeqsd         0x100000(%r15), %xmm12, %xmm11
	comltsd         0x100000(%r15), %xmm12, %xmm11
	comlesd         0x100000(%r15), %xmm12, %xmm11
	comunordsd      0x100000(%r15), %xmm12, %xmm11
	comnesd         0x100000(%r15), %xmm12, %xmm11
	comnltsd        0x100000(%r15), %xmm12, %xmm11
	comnlesd        0x100000(%r15), %xmm12, %xmm11
	comordsd        0x100000(%r15), %xmm12, %xmm11
	comueqsd        0x100000(%r15), %xmm12, %xmm11
	comultsd        0x100000(%r15), %xmm12, %xmm11
	comulesd        0x100000(%r15), %xmm12, %xmm11
	comfalsesd      0x100000(%r15), %xmm12, %xmm11
	comunesd        0x100000(%r15), %xmm12, %xmm11
	comunltsd       0x100000(%r15), %xmm12, %xmm11
	comunlesd       0x100000(%r15), %xmm12, %xmm11
	comtruesd       0x100000(%r15), %xmm12, %xmm11
	comps           $0x4, %xmm13, %xmm12, %xmm11
	comps           $0x4, 0x100000(%r15), %xmm12, %xmm11
	comeqps         %xmm13, %xmm12, %xmm11
	comltps         %xmm13, %xmm12, %xmm11
	comleps         %xmm13, %xmm12, %xmm11
	comunordps      %xmm13, %xmm12, %xmm11
	comneps         %xmm13, %xmm12, %xmm11
	comnltps        %xmm13, %xmm12, %xmm11
	comnleps        %xmm13, %xmm12, %xmm11
	comordps        %xmm13, %xmm12, %xmm11
	comueqps        %xmm13, %xmm12, %xmm11
	comultps        %xmm13, %xmm12, %xmm11
	comuleps        %xmm13, %xmm12, %xmm11
	comfalseps      %xmm13, %xmm12, %xmm11
	comuneps        %xmm13, %xmm12, %xmm11
	comunltps       %xmm13, %xmm12, %xmm11
	comunleps       %xmm13, %xmm12, %xmm11
	comtrueps       %xmm13, %xmm12, %xmm11
	comeqps         0x100000(%r15), %xmm12, %xmm11
	comltps         0x100000(%r15), %xmm12, %xmm11
	comleps         0x100000(%r15), %xmm12, %xmm11
	comunordps      0x100000(%r15), %xmm12, %xmm11
	comneps         0x100000(%r15), %xmm12, %xmm11
	comnltps        0x100000(%r15), %xmm12, %xmm11
	comnleps        0x100000(%r15), %xmm12, %xmm11
	comordps        0x100000(%r15), %xmm12, %xmm11
	comueqps        0x100000(%r15), %xmm12, %xmm11
	comultps        0x100000(%r15), %xmm12, %xmm11
	comuleps        0x100000(%r15), %xmm12, %xmm11
	comfalseps      0x100000(%r15), %xmm12, %xmm11
	comuneps        0x100000(%r15), %xmm12, %xmm11
	comunltps       0x100000(%r15), %xmm12, %xmm11
	comunleps       0x100000(%r15), %xmm12, %xmm11
	comtrueps       0x100000(%r15), %xmm12, %xmm11
	compd           $0x4, %xmm13, %xmm12, %xmm11
	compd           $0x4, 0x100000(%r15), %xmm12, %xmm11
	comeqpd         %xmm13, %xmm12, %xmm11
	comltpd         %xmm13, %xmm12, %xmm11
	comlepd         %xmm13, %xmm12, %xmm11
	comunordpd      %xmm13, %xmm12, %xmm11
	comnepd         %xmm13, %xmm12, %xmm11
	comnltpd        %xmm13, %xmm12, %xmm11
	comnlepd        %xmm13, %xmm12, %xmm11
	comordpd        %xmm13, %xmm12, %xmm11
	comueqpd        %xmm13, %xmm12, %xmm11
	comultpd        %xmm13, %xmm12, %xmm11
	comulepd        %xmm13, %xmm12, %xmm11
	comfalsepd      %xmm13, %xmm12, %xmm11
	comunepd        %xmm13, %xmm12, %xmm11
	comunltpd       %xmm13, %xmm12, %xmm11
	comunlepd       %xmm13, %xmm12, %xmm11
	comtruepd       %xmm13, %xmm12, %xmm11
	comeqpd         0x100000(%r15), %xmm12, %xmm11
	comltpd         0x100000(%r15), %xmm12, %xmm11
	comlepd         0x100000(%r15), %xmm12, %xmm11
	comunordpd      0x100000(%r15), %xmm12, %xmm11
	comnepd         0x100000(%r15), %xmm12, %xmm11
	comnltpd        0x100000(%r15), %xmm12, %xmm11
	comnlepd        0x100000(%r15), %xmm12, %xmm11
	comordpd        0x100000(%r15), %xmm12, %xmm11
	comueqpd        0x100000(%r15), %xmm12, %xmm11
	comultpd        0x100000(%r15), %xmm12, %xmm11
	comulepd        0x100000(%r15), %xmm12, %xmm11
	comfalsepd      0x100000(%r15), %xmm12, %xmm11
	comunepd        0x100000(%r15), %xmm12, %xmm11
	comunltpd       0x100000(%r15), %xmm12, %xmm11
	comunlepd       0x100000(%r15), %xmm12, %xmm11
	comtruepd       0x100000(%r15), %xmm12, %xmm11

	pcomb           $0x4, %xmm13, %xmm12, %xmm11
	pcomb           $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltb         %xmm13, %xmm12, %xmm11
	pcomleb         %xmm13, %xmm12, %xmm11
	pcomgtb         %xmm13, %xmm12, %xmm11
	pcomgeb         %xmm13, %xmm12, %xmm11
	pcomeqb         %xmm13, %xmm12, %xmm11
	pcomneqb        %xmm13, %xmm12, %xmm11
	pcomltb         0x100000(%r15), %xmm12, %xmm11
	pcomleb         0x100000(%r15), %xmm12, %xmm11
	pcomgtb         0x100000(%r15), %xmm12, %xmm11
	pcomgeb         0x100000(%r15), %xmm12, %xmm11
	pcomeqb         0x100000(%r15), %xmm12, %xmm11
	pcomneqb        0x100000(%r15), %xmm12, %xmm11
	pcomw           $0x4, %xmm13, %xmm12, %xmm11
	pcomw           $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltw         %xmm13, %xmm12, %xmm11
	pcomlew         %xmm13, %xmm12, %xmm11
	pcomgtw         %xmm13, %xmm12, %xmm11
	pcomgew         %xmm13, %xmm12, %xmm11
	pcomeqw         %xmm13, %xmm12, %xmm11
	pcomneqw        %xmm13, %xmm12, %xmm11
	pcomltw         0x100000(%r15), %xmm12, %xmm11
	pcomlew         0x100000(%r15), %xmm12, %xmm11
	pcomgtw         0x100000(%r15), %xmm12, %xmm11
	pcomgew         0x100000(%r15), %xmm12, %xmm11
	pcomeqw         0x100000(%r15), %xmm12, %xmm11
	pcomneqw        0x100000(%r15), %xmm12, %xmm11
	pcomd           $0x4, %xmm13, %xmm12, %xmm11
	pcomd           $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltd         %xmm13, %xmm12, %xmm11
	pcomled         %xmm13, %xmm12, %xmm11
	pcomgtd         %xmm13, %xmm12, %xmm11
	pcomged         %xmm13, %xmm12, %xmm11
	pcomeqd         %xmm13, %xmm12, %xmm11
	pcomneqd        %xmm13, %xmm12, %xmm11
	pcomltd         0x100000(%r15), %xmm12, %xmm11
	pcomled         0x100000(%r15), %xmm12, %xmm11
	pcomgtd         0x100000(%r15), %xmm12, %xmm11
	pcomged         0x100000(%r15), %xmm12, %xmm11
	pcomeqd         0x100000(%r15), %xmm12, %xmm11
	pcomneqd        0x100000(%r15), %xmm12, %xmm11
	pcomq           $0x4, %xmm13, %xmm12, %xmm11
	pcomq           $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltq         %xmm13, %xmm12, %xmm11
	pcomleq         %xmm13, %xmm12, %xmm11
	pcomgtq         %xmm13, %xmm12, %xmm11
	pcomgeq         %xmm13, %xmm12, %xmm11
	pcomeqq         %xmm13, %xmm12, %xmm11
	pcomneqq        %xmm13, %xmm12, %xmm11
	pcomltq         0x100000(%r15), %xmm12, %xmm11
	pcomleq         0x100000(%r15), %xmm12, %xmm11
	pcomgtq         0x100000(%r15), %xmm12, %xmm11
	pcomgeq         0x100000(%r15), %xmm12, %xmm11
	pcomeqq         0x100000(%r15), %xmm12, %xmm11
	pcomneqq        0x100000(%r15), %xmm12, %xmm11

	pcomub          $0x4, %xmm13, %xmm12, %xmm11
	pcomub          $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltub        %xmm13, %xmm12, %xmm11
	pcomleub        %xmm13, %xmm12, %xmm11
	pcomgtub        %xmm13, %xmm12, %xmm11
	pcomgeub        %xmm13, %xmm12, %xmm11
	pcomequb        %xmm13, %xmm12, %xmm11
	pcomnequb       %xmm13, %xmm12, %xmm11
	pcomltub        0x100000(%r15), %xmm12, %xmm11
	pcomleub        0x100000(%r15), %xmm12, %xmm11
	pcomgtub        0x100000(%r15), %xmm12, %xmm11
	pcomgeub        0x100000(%r15), %xmm12, %xmm11
	pcomequb        0x100000(%r15), %xmm12, %xmm11
	pcomnequb       0x100000(%r15), %xmm12, %xmm11
	pcomuw          $0x4, %xmm13, %xmm12, %xmm11
	pcomuw          $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltuw        %xmm13, %xmm12, %xmm11
	pcomleuw        %xmm13, %xmm12, %xmm11
	pcomgtuw        %xmm13, %xmm12, %xmm11
	pcomgeuw        %xmm13, %xmm12, %xmm11
	pcomequw        %xmm13, %xmm12, %xmm11
	pcomnequw       %xmm13, %xmm12, %xmm11
	pcomltuw        0x100000(%r15), %xmm12, %xmm11
	pcomleuw        0x100000(%r15), %xmm12, %xmm11
	pcomgtuw        0x100000(%r15), %xmm12, %xmm11
	pcomgeuw        0x100000(%r15), %xmm12, %xmm11
	pcomequw        0x100000(%r15), %xmm12, %xmm11
	pcomnequw       0x100000(%r15), %xmm12, %xmm11
	pcomud          $0x4, %xmm13, %xmm12, %xmm11
	pcomud          $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltud        %xmm13, %xmm12, %xmm11
	pcomleud        %xmm13, %xmm12, %xmm11
	pcomgtud        %xmm13, %xmm12, %xmm11
	pcomgeud        %xmm13, %xmm12, %xmm11
	pcomequd        %xmm13, %xmm12, %xmm11
	pcomnequd       %xmm13, %xmm12, %xmm11
	pcomltud        0x100000(%r15), %xmm12, %xmm11
	pcomleud        0x100000(%r15), %xmm12, %xmm11
	pcomgtud        0x100000(%r15), %xmm12, %xmm11
	pcomgeud        0x100000(%r15), %xmm12, %xmm11
	pcomequd        0x100000(%r15), %xmm12, %xmm11
	pcomnequd       0x100000(%r15), %xmm12, %xmm11
	pcomuq          $0x4, %xmm13, %xmm12, %xmm11
	pcomuq          $0x4, 0x100000(%r15), %xmm12, %xmm11
	pcomltuq        %xmm13, %xmm12, %xmm11
	pcomleuq        %xmm13, %xmm12, %xmm11
	pcomgtuq        %xmm13, %xmm12, %xmm11
	pcomgeuq        %xmm13, %xmm12, %xmm11
	pcomequq        %xmm13, %xmm12, %xmm11
	pcomnequq       %xmm13, %xmm12, %xmm11
	pcomltuq        0x100000(%r15), %xmm12, %xmm11
	pcomleuq        0x100000(%r15), %xmm12, %xmm11
	pcomgtuq        0x100000(%r15), %xmm12, %xmm11
	pcomgeuq        0x100000(%r15), %xmm12, %xmm11
	pcomequq        0x100000(%r15), %xmm12, %xmm11
	pcomnequq       0x100000(%r15), %xmm12, %xmm11

	comss           $0x4, %xmm3, %xmm12, %xmm1
	comss           $0x4, 0x4(%rdx), %xmm12, %xmm1
	comeqss         %xmm3, %xmm12, %xmm1
	comltss         %xmm3, %xmm12, %xmm1
	comless         %xmm3, %xmm12, %xmm1
	comunordss      %xmm3, %xmm12, %xmm1
	comness         %xmm3, %xmm12, %xmm1
	comnltss        %xmm3, %xmm12, %xmm1
	comnless        %xmm3, %xmm12, %xmm1
	comordss        %xmm3, %xmm12, %xmm1
	comueqss        %xmm3, %xmm12, %xmm1
	comultss        %xmm3, %xmm12, %xmm1
	comuless        %xmm3, %xmm12, %xmm1
	comfalsess      %xmm3, %xmm12, %xmm1
	comuness        %xmm3, %xmm12, %xmm1
	comunltss       %xmm3, %xmm12, %xmm1
	comunless       %xmm3, %xmm12, %xmm1
	comtruess       %xmm3, %xmm12, %xmm1
	comeqss         0x4(%rdx), %xmm12, %xmm1
	comltss         0x4(%rdx), %xmm12, %xmm1
	comless         0x4(%rdx), %xmm12, %xmm1
	comunordss      0x4(%rdx), %xmm12, %xmm1
	comness         0x4(%rdx), %xmm12, %xmm1
	comnltss        0x4(%rdx), %xmm12, %xmm1
	comnless        0x4(%rdx), %xmm12, %xmm1
	comordss        0x4(%rdx), %xmm12, %xmm1
	comueqss        0x4(%rdx), %xmm12, %xmm1
	comultss        0x4(%rdx), %xmm12, %xmm1
	comuless        0x4(%rdx), %xmm12, %xmm1
	comfalsess      0x4(%rdx), %xmm12, %xmm1
	comuness        0x4(%rdx), %xmm12, %xmm1
	comunltss       0x4(%rdx), %xmm12, %xmm1
	comunless       0x4(%rdx), %xmm12, %xmm1
	comtruess       0x4(%rdx), %xmm12, %xmm1
	comsd           $0x4, %xmm3, %xmm12, %xmm1
	comsd           $0x4, 0x4(%rdx), %xmm12, %xmm1
	comeqsd         %xmm3, %xmm12, %xmm1
	comltsd         %xmm3, %xmm12, %xmm1
	comlesd         %xmm3, %xmm12, %xmm1
	comunordsd      %xmm3, %xmm12, %xmm1
	comnesd         %xmm3, %xmm12, %xmm1
	comnltsd        %xmm3, %xmm12, %xmm1
	comnlesd        %xmm3, %xmm12, %xmm1
	comordsd        %xmm3, %xmm12, %xmm1
	comueqsd        %xmm3, %xmm12, %xmm1
	comultsd        %xmm3, %xmm12, %xmm1
	comulesd        %xmm3, %xmm12, %xmm1
	comfalsesd      %xmm3, %xmm12, %xmm1
	comunesd        %xmm3, %xmm12, %xmm1
	comunltsd       %xmm3, %xmm12, %xmm1
	comunlesd       %xmm3, %xmm12, %xmm1
	comtruesd       %xmm3, %xmm12, %xmm1
	comeqsd         0x4(%rdx), %xmm12, %xmm1
	comltsd         0x4(%rdx), %xmm12, %xmm1
	comlesd         0x4(%rdx), %xmm12, %xmm1
	comunordsd      0x4(%rdx), %xmm12, %xmm1
	comnesd         0x4(%rdx), %xmm12, %xmm1
	comnltsd        0x4(%rdx), %xmm12, %xmm1
	comnlesd        0x4(%rdx), %xmm12, %xmm1
	comordsd        0x4(%rdx), %xmm12, %xmm1
	comueqsd        0x4(%rdx), %xmm12, %xmm1
	comultsd        0x4(%rdx), %xmm12, %xmm1
	comulesd        0x4(%rdx), %xmm12, %xmm1
	comfalsesd      0x4(%rdx), %xmm12, %xmm1
	comunesd        0x4(%rdx), %xmm12, %xmm1
	comunltsd       0x4(%rdx), %xmm12, %xmm1
	comunlesd       0x4(%rdx), %xmm12, %xmm1
	comtruesd       0x4(%rdx), %xmm12, %xmm1
	comps           $0x4, %xmm3, %xmm12, %xmm1
	comps           $0x4, 0x4(%rdx), %xmm12, %xmm1
	comeqps         %xmm3, %xmm12, %xmm1
	comltps         %xmm3, %xmm12, %xmm1
	comleps         %xmm3, %xmm12, %xmm1
	comunordps      %xmm3, %xmm12, %xmm1
	comneps         %xmm3, %xmm12, %xmm1
	comnltps        %xmm3, %xmm12, %xmm1
	comnleps        %xmm3, %xmm12, %xmm1
	comordps        %xmm3, %xmm12, %xmm1
	comueqps        %xmm3, %xmm12, %xmm1
	comultps        %xmm3, %xmm12, %xmm1
	comuleps        %xmm3, %xmm12, %xmm1
	comfalseps      %xmm3, %xmm12, %xmm1
	comuneps        %xmm3, %xmm12, %xmm1
	comunltps       %xmm3, %xmm12, %xmm1
	comunleps       %xmm3, %xmm12, %xmm1
	comtrueps       %xmm3, %xmm12, %xmm1
	comeqps         0x4(%rdx), %xmm12, %xmm1
	comltps         0x4(%rdx), %xmm12, %xmm1
	comleps         0x4(%rdx), %xmm12, %xmm1
	comunordps      0x4(%rdx), %xmm12, %xmm1
	comneps         0x4(%rdx), %xmm12, %xmm1
	comnltps        0x4(%rdx), %xmm12, %xmm1
	comnleps        0x4(%rdx), %xmm12, %xmm1
	comordps        0x4(%rdx), %xmm12, %xmm1
	comueqps        0x4(%rdx), %xmm12, %xmm1
	comultps        0x4(%rdx), %xmm12, %xmm1
	comuleps        0x4(%rdx), %xmm12, %xmm1
	comfalseps      0x4(%rdx), %xmm12, %xmm1
	comuneps        0x4(%rdx), %xmm12, %xmm1
	comunltps       0x4(%rdx), %xmm12, %xmm1
	comunleps       0x4(%rdx), %xmm12, %xmm1
	comtrueps       0x4(%rdx), %xmm12, %xmm1
	compd           $0x4, %xmm3, %xmm12, %xmm1
	compd           $0x4, 0x4(%rdx), %xmm12, %xmm1
	comeqpd         %xmm3, %xmm12, %xmm1
	comltpd         %xmm3, %xmm12, %xmm1
	comlepd         %xmm3, %xmm12, %xmm1
	comunordpd      %xmm3, %xmm12, %xmm1
	comnepd         %xmm3, %xmm12, %xmm1
	comnltpd        %xmm3, %xmm12, %xmm1
	comnlepd        %xmm3, %xmm12, %xmm1
	comordpd        %xmm3, %xmm12, %xmm1
	comueqpd        %xmm3, %xmm12, %xmm1
	comultpd        %xmm3, %xmm12, %xmm1
	comulepd        %xmm3, %xmm12, %xmm1
	comfalsepd      %xmm3, %xmm12, %xmm1
	comunepd        %xmm3, %xmm12, %xmm1
	comunltpd       %xmm3, %xmm12, %xmm1
	comunlepd       %xmm3, %xmm12, %xmm1
	comtruepd       %xmm3, %xmm12, %xmm1
	comeqpd         0x4(%rdx), %xmm12, %xmm1
	comltpd         0x4(%rdx), %xmm12, %xmm1
	comlepd         0x4(%rdx), %xmm12, %xmm1
	comunordpd      0x4(%rdx), %xmm12, %xmm1
	comnepd         0x4(%rdx), %xmm12, %xmm1
	comnltpd        0x4(%rdx), %xmm12, %xmm1
	comnlepd        0x4(%rdx), %xmm12, %xmm1
	comordpd        0x4(%rdx), %xmm12, %xmm1
	comueqpd        0x4(%rdx), %xmm12, %xmm1
	comultpd        0x4(%rdx), %xmm12, %xmm1
	comulepd        0x4(%rdx), %xmm12, %xmm1
	comfalsepd      0x4(%rdx), %xmm12, %xmm1
	comunepd        0x4(%rdx), %xmm12, %xmm1
	comunltpd       0x4(%rdx), %xmm12, %xmm1
	comunlepd       0x4(%rdx), %xmm12, %xmm1
	comtruepd       0x4(%rdx), %xmm12, %xmm1

	pcomb           $0x4, %xmm3, %xmm12, %xmm1
	pcomb           $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltb         %xmm3, %xmm12, %xmm1
	pcomleb         %xmm3, %xmm12, %xmm1
	pcomgtb         %xmm3, %xmm12, %xmm1
	pcomgeb         %xmm3, %xmm12, %xmm1
	pcomeqb         %xmm3, %xmm12, %xmm1
	pcomneqb        %xmm3, %xmm12, %xmm1
	pcomltb         0x4(%rdx), %xmm12, %xmm1
	pcomleb         0x4(%rdx), %xmm12, %xmm1
	pcomgtb         0x4(%rdx), %xmm12, %xmm1
	pcomgeb         0x4(%rdx), %xmm12, %xmm1
	pcomeqb         0x4(%rdx), %xmm12, %xmm1
	pcomneqb        0x4(%rdx), %xmm12, %xmm1
	pcomw           $0x4, %xmm3, %xmm12, %xmm1
	pcomw           $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltw         %xmm3, %xmm12, %xmm1
	pcomlew         %xmm3, %xmm12, %xmm1
	pcomgtw         %xmm3, %xmm12, %xmm1
	pcomgew         %xmm3, %xmm12, %xmm1
	pcomeqw         %xmm3, %xmm12, %xmm1
	pcomneqw        %xmm3, %xmm12, %xmm1
	pcomltw         0x4(%rdx), %xmm12, %xmm1
	pcomlew         0x4(%rdx), %xmm12, %xmm1
	pcomgtw         0x4(%rdx), %xmm12, %xmm1
	pcomgew         0x4(%rdx), %xmm12, %xmm1
	pcomeqw         0x4(%rdx), %xmm12, %xmm1
	pcomneqw        0x4(%rdx), %xmm12, %xmm1
	pcomd           $0x4, %xmm3, %xmm12, %xmm1
	pcomd           $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltd         %xmm3, %xmm12, %xmm1
	pcomled         %xmm3, %xmm12, %xmm1
	pcomgtd         %xmm3, %xmm12, %xmm1
	pcomged         %xmm3, %xmm12, %xmm1
	pcomeqd         %xmm3, %xmm12, %xmm1
	pcomneqd        %xmm3, %xmm12, %xmm1
	pcomltd         0x4(%rdx), %xmm12, %xmm1
	pcomled         0x4(%rdx), %xmm12, %xmm1
	pcomgtd         0x4(%rdx), %xmm12, %xmm1
	pcomged         0x4(%rdx), %xmm12, %xmm1
	pcomeqd         0x4(%rdx), %xmm12, %xmm1
	pcomneqd        0x4(%rdx), %xmm12, %xmm1
	pcomq           $0x4, %xmm3, %xmm12, %xmm1
	pcomq           $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltq         %xmm3, %xmm12, %xmm1
	pcomleq         %xmm3, %xmm12, %xmm1
	pcomgtq         %xmm3, %xmm12, %xmm1
	pcomgeq         %xmm3, %xmm12, %xmm1
	pcomeqq         %xmm3, %xmm12, %xmm1
	pcomneqq        %xmm3, %xmm12, %xmm1
	pcomltq         0x4(%rdx), %xmm12, %xmm1
	pcomleq         0x4(%rdx), %xmm12, %xmm1
	pcomgtq         0x4(%rdx), %xmm12, %xmm1
	pcomgeq         0x4(%rdx), %xmm12, %xmm1
	pcomeqq         0x4(%rdx), %xmm12, %xmm1
	pcomneqq        0x4(%rdx), %xmm12, %xmm1

	pcomub          $0x4, %xmm3, %xmm12, %xmm1
	pcomub          $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltub        %xmm3, %xmm12, %xmm1
	pcomleub        %xmm3, %xmm12, %xmm1
	pcomgtub        %xmm3, %xmm12, %xmm1
	pcomgeub        %xmm3, %xmm12, %xmm1
	pcomequb        %xmm3, %xmm12, %xmm1
	pcomnequb       %xmm3, %xmm12, %xmm1
	pcomltub        0x4(%rdx), %xmm12, %xmm1
	pcomleub        0x4(%rdx), %xmm12, %xmm1
	pcomgtub        0x4(%rdx), %xmm12, %xmm1
	pcomgeub        0x4(%rdx), %xmm12, %xmm1
	pcomequb        0x4(%rdx), %xmm12, %xmm1
	pcomnequb       0x4(%rdx), %xmm12, %xmm1
	pcomuw          $0x4, %xmm3, %xmm12, %xmm1
	pcomuw          $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltuw        %xmm3, %xmm12, %xmm1
	pcomleuw        %xmm3, %xmm12, %xmm1
	pcomgtuw        %xmm3, %xmm12, %xmm1
	pcomgeuw        %xmm3, %xmm12, %xmm1
	pcomequw        %xmm3, %xmm12, %xmm1
	pcomnequw       %xmm3, %xmm12, %xmm1
	pcomltuw        0x4(%rdx), %xmm12, %xmm1
	pcomleuw        0x4(%rdx), %xmm12, %xmm1
	pcomgtuw        0x4(%rdx), %xmm12, %xmm1
	pcomgeuw        0x4(%rdx), %xmm12, %xmm1
	pcomequw        0x4(%rdx), %xmm12, %xmm1
	pcomnequw       0x4(%rdx), %xmm12, %xmm1
	pcomud          $0x4, %xmm3, %xmm12, %xmm1
	pcomud          $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltud        %xmm3, %xmm12, %xmm1
	pcomleud        %xmm3, %xmm12, %xmm1
	pcomgtud        %xmm3, %xmm12, %xmm1
	pcomgeud        %xmm3, %xmm12, %xmm1
	pcomequd        %xmm3, %xmm12, %xmm1
	pcomnequd       %xmm3, %xmm12, %xmm1
	pcomltud        0x4(%rdx), %xmm12, %xmm1
	pcomleud        0x4(%rdx), %xmm12, %xmm1
	pcomgtud        0x4(%rdx), %xmm12, %xmm1
	pcomgeud        0x4(%rdx), %xmm12, %xmm1
	pcomequd        0x4(%rdx), %xmm12, %xmm1
	pcomnequd       0x4(%rdx), %xmm12, %xmm1
	pcomuq          $0x4, %xmm3, %xmm12, %xmm1
	pcomuq          $0x4, 0x4(%rdx), %xmm12, %xmm1
	pcomltuq        %xmm3, %xmm12, %xmm1
	pcomleuq        %xmm3, %xmm12, %xmm1
	pcomgtuq        %xmm3, %xmm12, %xmm1
	pcomgeuq        %xmm3, %xmm12, %xmm1
	pcomequq        %xmm3, %xmm12, %xmm1
	pcomnequq       %xmm3, %xmm12, %xmm1
	pcomltuq        0x4(%rdx), %xmm12, %xmm1
	pcomleuq        0x4(%rdx), %xmm12, %xmm1
	pcomgtuq        0x4(%rdx), %xmm12, %xmm1
	pcomgeuq        0x4(%rdx), %xmm12, %xmm1
	pcomequq        0x4(%rdx), %xmm12, %xmm1
	pcomnequq       0x4(%rdx), %xmm12, %xmm1

	comss           $0x4, %xmm3, %xmm2, %xmm11
	comss           $0x4, 0x4(%rdx), %xmm2, %xmm11
	comeqss         %xmm3, %xmm2, %xmm11
	comltss         %xmm3, %xmm2, %xmm11
	comless         %xmm3, %xmm2, %xmm11
	comunordss      %xmm3, %xmm2, %xmm11
	comness         %xmm3, %xmm2, %xmm11
	comnltss        %xmm3, %xmm2, %xmm11
	comnless        %xmm3, %xmm2, %xmm11
	comordss        %xmm3, %xmm2, %xmm11
	comueqss        %xmm3, %xmm2, %xmm11
	comultss        %xmm3, %xmm2, %xmm11
	comuless        %xmm3, %xmm2, %xmm11
	comfalsess      %xmm3, %xmm2, %xmm11
	comuness        %xmm3, %xmm2, %xmm11
	comunltss       %xmm3, %xmm2, %xmm11
	comunless       %xmm3, %xmm2, %xmm11
	comtruess       %xmm3, %xmm2, %xmm11
	comeqss         0x4(%rdx), %xmm2, %xmm11
	comltss         0x4(%rdx), %xmm2, %xmm11
	comless         0x4(%rdx), %xmm2, %xmm11
	comunordss      0x4(%rdx), %xmm2, %xmm11
	comness         0x4(%rdx), %xmm2, %xmm11
	comnltss        0x4(%rdx), %xmm2, %xmm11
	comnless        0x4(%rdx), %xmm2, %xmm11
	comordss        0x4(%rdx), %xmm2, %xmm11
	comueqss        0x4(%rdx), %xmm2, %xmm11
	comultss        0x4(%rdx), %xmm2, %xmm11
	comuless        0x4(%rdx), %xmm2, %xmm11
	comfalsess      0x4(%rdx), %xmm2, %xmm11
	comuness        0x4(%rdx), %xmm2, %xmm11
	comunltss       0x4(%rdx), %xmm2, %xmm11
	comunless       0x4(%rdx), %xmm2, %xmm11
	comtruess       0x4(%rdx), %xmm2, %xmm11
	comsd           $0x4, %xmm3, %xmm2, %xmm11
	comsd           $0x4, 0x4(%rdx), %xmm2, %xmm11
	comeqsd         %xmm3, %xmm2, %xmm11
	comltsd         %xmm3, %xmm2, %xmm11
	comlesd         %xmm3, %xmm2, %xmm11
	comunordsd      %xmm3, %xmm2, %xmm11
	comnesd         %xmm3, %xmm2, %xmm11
	comnltsd        %xmm3, %xmm2, %xmm11
	comnlesd        %xmm3, %xmm2, %xmm11
	comordsd        %xmm3, %xmm2, %xmm11
	comueqsd        %xmm3, %xmm2, %xmm11
	comultsd        %xmm3, %xmm2, %xmm11
	comulesd        %xmm3, %xmm2, %xmm11
	comfalsesd      %xmm3, %xmm2, %xmm11
	comunesd        %xmm3, %xmm2, %xmm11
	comunltsd       %xmm3, %xmm2, %xmm11
	comunlesd       %xmm3, %xmm2, %xmm11
	comtruesd       %xmm3, %xmm2, %xmm11
	comeqsd         0x4(%rdx), %xmm2, %xmm11
	comltsd         0x4(%rdx), %xmm2, %xmm11
	comlesd         0x4(%rdx), %xmm2, %xmm11
	comunordsd      0x4(%rdx), %xmm2, %xmm11
	comnesd         0x4(%rdx), %xmm2, %xmm11
	comnltsd        0x4(%rdx), %xmm2, %xmm11
	comnlesd        0x4(%rdx), %xmm2, %xmm11
	comordsd        0x4(%rdx), %xmm2, %xmm11
	comueqsd        0x4(%rdx), %xmm2, %xmm11
	comultsd        0x4(%rdx), %xmm2, %xmm11
	comulesd        0x4(%rdx), %xmm2, %xmm11
	comfalsesd      0x4(%rdx), %xmm2, %xmm11
	comunesd        0x4(%rdx), %xmm2, %xmm11
	comunltsd       0x4(%rdx), %xmm2, %xmm11
	comunlesd       0x4(%rdx), %xmm2, %xmm11
	comtruesd       0x4(%rdx), %xmm2, %xmm11
	comps           $0x4, %xmm3, %xmm2, %xmm11
	comps           $0x4, 0x4(%rdx), %xmm2, %xmm11
	comeqps         %xmm3, %xmm2, %xmm11
	comltps         %xmm3, %xmm2, %xmm11
	comleps         %xmm3, %xmm2, %xmm11
	comunordps      %xmm3, %xmm2, %xmm11
	comneps         %xmm3, %xmm2, %xmm11
	comnltps        %xmm3, %xmm2, %xmm11
	comnleps        %xmm3, %xmm2, %xmm11
	comordps        %xmm3, %xmm2, %xmm11
	comueqps        %xmm3, %xmm2, %xmm11
	comultps        %xmm3, %xmm2, %xmm11
	comuleps        %xmm3, %xmm2, %xmm11
	comfalseps      %xmm3, %xmm2, %xmm11
	comuneps        %xmm3, %xmm2, %xmm11
	comunltps       %xmm3, %xmm2, %xmm11
	comunleps       %xmm3, %xmm2, %xmm11
	comtrueps       %xmm3, %xmm2, %xmm11
	comeqps         0x4(%rdx), %xmm2, %xmm11
	comltps         0x4(%rdx), %xmm2, %xmm11
	comleps         0x4(%rdx), %xmm2, %xmm11
	comunordps      0x4(%rdx), %xmm2, %xmm11
	comneps         0x4(%rdx), %xmm2, %xmm11
	comnltps        0x4(%rdx), %xmm2, %xmm11
	comnleps        0x4(%rdx), %xmm2, %xmm11
	comordps        0x4(%rdx), %xmm2, %xmm11
	comueqps        0x4(%rdx), %xmm2, %xmm11
	comultps        0x4(%rdx), %xmm2, %xmm11
	comuleps        0x4(%rdx), %xmm2, %xmm11
	comfalseps      0x4(%rdx), %xmm2, %xmm11
	comuneps        0x4(%rdx), %xmm2, %xmm11
	comunltps       0x4(%rdx), %xmm2, %xmm11
	comunleps       0x4(%rdx), %xmm2, %xmm11
	comtrueps       0x4(%rdx), %xmm2, %xmm11
	compd           $0x4, %xmm3, %xmm2, %xmm11
	compd           $0x4, 0x4(%rdx), %xmm2, %xmm11
	comeqpd         %xmm3, %xmm2, %xmm11
	comltpd         %xmm3, %xmm2, %xmm11
	comlepd         %xmm3, %xmm2, %xmm11
	comunordpd      %xmm3, %xmm2, %xmm11
	comnepd         %xmm3, %xmm2, %xmm11
	comnltpd        %xmm3, %xmm2, %xmm11
	comnlepd        %xmm3, %xmm2, %xmm11
	comordpd        %xmm3, %xmm2, %xmm11
	comueqpd        %xmm3, %xmm2, %xmm11
	comultpd        %xmm3, %xmm2, %xmm11
	comulepd        %xmm3, %xmm2, %xmm11
	comfalsepd      %xmm3, %xmm2, %xmm11
	comunepd        %xmm3, %xmm2, %xmm11
	comunltpd       %xmm3, %xmm2, %xmm11
	comunlepd       %xmm3, %xmm2, %xmm11
	comtruepd       %xmm3, %xmm2, %xmm11
	comeqpd         0x4(%rdx), %xmm2, %xmm11
	comltpd         0x4(%rdx), %xmm2, %xmm11
	comlepd         0x4(%rdx), %xmm2, %xmm11
	comunordpd      0x4(%rdx), %xmm2, %xmm11
	comnepd         0x4(%rdx), %xmm2, %xmm11
	comnltpd        0x4(%rdx), %xmm2, %xmm11
	comnlepd        0x4(%rdx), %xmm2, %xmm11
	comordpd        0x4(%rdx), %xmm2, %xmm11
	comueqpd        0x4(%rdx), %xmm2, %xmm11
	comultpd        0x4(%rdx), %xmm2, %xmm11
	comulepd        0x4(%rdx), %xmm2, %xmm11
	comfalsepd      0x4(%rdx), %xmm2, %xmm11
	comunepd        0x4(%rdx), %xmm2, %xmm11
	comunltpd       0x4(%rdx), %xmm2, %xmm11
	comunlepd       0x4(%rdx), %xmm2, %xmm11
	comtruepd       0x4(%rdx), %xmm2, %xmm11

	pcomb           $0x4, %xmm3, %xmm2, %xmm11
	pcomb           $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltb         %xmm3, %xmm2, %xmm11
	pcomleb         %xmm3, %xmm2, %xmm11
	pcomgtb         %xmm3, %xmm2, %xmm11
	pcomgeb         %xmm3, %xmm2, %xmm11
	pcomeqb         %xmm3, %xmm2, %xmm11
	pcomneqb        %xmm3, %xmm2, %xmm11
	pcomltb         0x4(%rdx), %xmm2, %xmm11
	pcomleb         0x4(%rdx), %xmm2, %xmm11
	pcomgtb         0x4(%rdx), %xmm2, %xmm11
	pcomgeb         0x4(%rdx), %xmm2, %xmm11
	pcomeqb         0x4(%rdx), %xmm2, %xmm11
	pcomneqb        0x4(%rdx), %xmm2, %xmm11
	pcomw           $0x4, %xmm3, %xmm2, %xmm11
	pcomw           $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltw         %xmm3, %xmm2, %xmm11
	pcomlew         %xmm3, %xmm2, %xmm11
	pcomgtw         %xmm3, %xmm2, %xmm11
	pcomgew         %xmm3, %xmm2, %xmm11
	pcomeqw         %xmm3, %xmm2, %xmm11
	pcomneqw        %xmm3, %xmm2, %xmm11
	pcomltw         0x4(%rdx), %xmm2, %xmm11
	pcomlew         0x4(%rdx), %xmm2, %xmm11
	pcomgtw         0x4(%rdx), %xmm2, %xmm11
	pcomgew         0x4(%rdx), %xmm2, %xmm11
	pcomeqw         0x4(%rdx), %xmm2, %xmm11
	pcomneqw        0x4(%rdx), %xmm2, %xmm11
	pcomd           $0x4, %xmm3, %xmm2, %xmm11
	pcomd           $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltd         %xmm3, %xmm2, %xmm11
	pcomled         %xmm3, %xmm2, %xmm11
	pcomgtd         %xmm3, %xmm2, %xmm11
	pcomged         %xmm3, %xmm2, %xmm11
	pcomeqd         %xmm3, %xmm2, %xmm11
	pcomneqd        %xmm3, %xmm2, %xmm11
	pcomltd         0x4(%rdx), %xmm2, %xmm11
	pcomled         0x4(%rdx), %xmm2, %xmm11
	pcomgtd         0x4(%rdx), %xmm2, %xmm11
	pcomged         0x4(%rdx), %xmm2, %xmm11
	pcomeqd         0x4(%rdx), %xmm2, %xmm11
	pcomneqd        0x4(%rdx), %xmm2, %xmm11
	pcomq           $0x4, %xmm3, %xmm2, %xmm11
	pcomq           $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltq         %xmm3, %xmm2, %xmm11
	pcomleq         %xmm3, %xmm2, %xmm11
	pcomgtq         %xmm3, %xmm2, %xmm11
	pcomgeq         %xmm3, %xmm2, %xmm11
	pcomeqq         %xmm3, %xmm2, %xmm11
	pcomneqq        %xmm3, %xmm2, %xmm11
	pcomltq         0x4(%rdx), %xmm2, %xmm11
	pcomleq         0x4(%rdx), %xmm2, %xmm11
	pcomgtq         0x4(%rdx), %xmm2, %xmm11
	pcomgeq         0x4(%rdx), %xmm2, %xmm11
	pcomeqq         0x4(%rdx), %xmm2, %xmm11
	pcomneqq        0x4(%rdx), %xmm2, %xmm11

	pcomub          $0x4, %xmm3, %xmm2, %xmm11
	pcomub          $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltub        %xmm3, %xmm2, %xmm11
	pcomleub        %xmm3, %xmm2, %xmm11
	pcomgtub        %xmm3, %xmm2, %xmm11
	pcomgeub        %xmm3, %xmm2, %xmm11
	pcomequb        %xmm3, %xmm2, %xmm11
	pcomnequb       %xmm3, %xmm2, %xmm11
	pcomltub        0x4(%rdx), %xmm2, %xmm11
	pcomleub        0x4(%rdx), %xmm2, %xmm11
	pcomgtub        0x4(%rdx), %xmm2, %xmm11
	pcomgeub        0x4(%rdx), %xmm2, %xmm11
	pcomequb        0x4(%rdx), %xmm2, %xmm11
	pcomnequb       0x4(%rdx), %xmm2, %xmm11
	pcomuw          $0x4, %xmm3, %xmm2, %xmm11
	pcomuw          $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltuw        %xmm3, %xmm2, %xmm11
	pcomleuw        %xmm3, %xmm2, %xmm11
	pcomgtuw        %xmm3, %xmm2, %xmm11
	pcomgeuw        %xmm3, %xmm2, %xmm11
	pcomequw        %xmm3, %xmm2, %xmm11
	pcomnequw       %xmm3, %xmm2, %xmm11
	pcomltuw        0x4(%rdx), %xmm2, %xmm11
	pcomleuw        0x4(%rdx), %xmm2, %xmm11
	pcomgtuw        0x4(%rdx), %xmm2, %xmm11
	pcomgeuw        0x4(%rdx), %xmm2, %xmm11
	pcomequw        0x4(%rdx), %xmm2, %xmm11
	pcomnequw       0x4(%rdx), %xmm2, %xmm11
	pcomud          $0x4, %xmm3, %xmm2, %xmm11
	pcomud          $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltud        %xmm3, %xmm2, %xmm11
	pcomleud        %xmm3, %xmm2, %xmm11
	pcomgtud        %xmm3, %xmm2, %xmm11
	pcomgeud        %xmm3, %xmm2, %xmm11
	pcomequd        %xmm3, %xmm2, %xmm11
	pcomnequd       %xmm3, %xmm2, %xmm11
	pcomltud        0x4(%rdx), %xmm2, %xmm11
	pcomleud        0x4(%rdx), %xmm2, %xmm11
	pcomgtud        0x4(%rdx), %xmm2, %xmm11
	pcomgeud        0x4(%rdx), %xmm2, %xmm11
	pcomequd        0x4(%rdx), %xmm2, %xmm11
	pcomnequd       0x4(%rdx), %xmm2, %xmm11
	pcomuq          $0x4, %xmm3, %xmm2, %xmm11
	pcomuq          $0x4, 0x4(%rdx), %xmm2, %xmm11
	pcomltuq        %xmm3, %xmm2, %xmm11
	pcomleuq        %xmm3, %xmm2, %xmm11
	pcomgtuq        %xmm3, %xmm2, %xmm11
	pcomgeuq        %xmm3, %xmm2, %xmm11
	pcomequq        %xmm3, %xmm2, %xmm11
	pcomnequq       %xmm3, %xmm2, %xmm11
	pcomltuq        0x4(%rdx), %xmm2, %xmm11
	pcomleuq        0x4(%rdx), %xmm2, %xmm11
	pcomgtuq        0x4(%rdx), %xmm2, %xmm11
	pcomgeuq        0x4(%rdx), %xmm2, %xmm11
	pcomequq        0x4(%rdx), %xmm2, %xmm11
	pcomnequq       0x4(%rdx), %xmm2, %xmm11

	comss           $0x4, %xmm13, %xmm2, %xmm1
	comss           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqss         %xmm13, %xmm2, %xmm1
	comltss         %xmm13, %xmm2, %xmm1
	comless         %xmm13, %xmm2, %xmm1
	comunordss      %xmm13, %xmm2, %xmm1
	comness         %xmm13, %xmm2, %xmm1
	comnltss        %xmm13, %xmm2, %xmm1
	comnless        %xmm13, %xmm2, %xmm1
	comordss        %xmm13, %xmm2, %xmm1
	comueqss        %xmm13, %xmm2, %xmm1
	comultss        %xmm13, %xmm2, %xmm1
	comuless        %xmm13, %xmm2, %xmm1
	comfalsess      %xmm13, %xmm2, %xmm1
	comuness        %xmm13, %xmm2, %xmm1
	comunltss       %xmm13, %xmm2, %xmm1
	comunless       %xmm13, %xmm2, %xmm1
	comtruess       %xmm13, %xmm2, %xmm1
	comeqss         0x4(%rdx), %xmm2, %xmm1
	comltss         0x4(%rdx), %xmm2, %xmm1
	comless         0x4(%rdx), %xmm2, %xmm1
	comunordss      0x4(%rdx), %xmm2, %xmm1
	comness         0x4(%rdx), %xmm2, %xmm1
	comnltss        0x4(%rdx), %xmm2, %xmm1
	comnless        0x4(%rdx), %xmm2, %xmm1
	comordss        0x4(%rdx), %xmm2, %xmm1
	comueqss        0x4(%rdx), %xmm2, %xmm1
	comultss        0x4(%rdx), %xmm2, %xmm1
	comuless        0x4(%rdx), %xmm2, %xmm1
	comfalsess      0x4(%rdx), %xmm2, %xmm1
	comuness        0x4(%rdx), %xmm2, %xmm1
	comunltss       0x4(%rdx), %xmm2, %xmm1
	comunless       0x4(%rdx), %xmm2, %xmm1
	comtruess       0x4(%rdx), %xmm2, %xmm1
	comsd           $0x4, %xmm13, %xmm2, %xmm1
	comsd           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqsd         %xmm13, %xmm2, %xmm1
	comltsd         %xmm13, %xmm2, %xmm1
	comlesd         %xmm13, %xmm2, %xmm1
	comunordsd      %xmm13, %xmm2, %xmm1
	comnesd         %xmm13, %xmm2, %xmm1
	comnltsd        %xmm13, %xmm2, %xmm1
	comnlesd        %xmm13, %xmm2, %xmm1
	comordsd        %xmm13, %xmm2, %xmm1
	comueqsd        %xmm13, %xmm2, %xmm1
	comultsd        %xmm13, %xmm2, %xmm1
	comulesd        %xmm13, %xmm2, %xmm1
	comfalsesd      %xmm13, %xmm2, %xmm1
	comunesd        %xmm13, %xmm2, %xmm1
	comunltsd       %xmm13, %xmm2, %xmm1
	comunlesd       %xmm13, %xmm2, %xmm1
	comtruesd       %xmm13, %xmm2, %xmm1
	comeqsd         0x4(%rdx), %xmm2, %xmm1
	comltsd         0x4(%rdx), %xmm2, %xmm1
	comlesd         0x4(%rdx), %xmm2, %xmm1
	comunordsd      0x4(%rdx), %xmm2, %xmm1
	comnesd         0x4(%rdx), %xmm2, %xmm1
	comnltsd        0x4(%rdx), %xmm2, %xmm1
	comnlesd        0x4(%rdx), %xmm2, %xmm1
	comordsd        0x4(%rdx), %xmm2, %xmm1
	comueqsd        0x4(%rdx), %xmm2, %xmm1
	comultsd        0x4(%rdx), %xmm2, %xmm1
	comulesd        0x4(%rdx), %xmm2, %xmm1
	comfalsesd      0x4(%rdx), %xmm2, %xmm1
	comunesd        0x4(%rdx), %xmm2, %xmm1
	comunltsd       0x4(%rdx), %xmm2, %xmm1
	comunlesd       0x4(%rdx), %xmm2, %xmm1
	comtruesd       0x4(%rdx), %xmm2, %xmm1
	comps           $0x4, %xmm13, %xmm2, %xmm1
	comps           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqps         %xmm13, %xmm2, %xmm1
	comltps         %xmm13, %xmm2, %xmm1
	comleps         %xmm13, %xmm2, %xmm1
	comunordps      %xmm13, %xmm2, %xmm1
	comneps         %xmm13, %xmm2, %xmm1
	comnltps        %xmm13, %xmm2, %xmm1
	comnleps        %xmm13, %xmm2, %xmm1
	comordps        %xmm13, %xmm2, %xmm1
	comueqps        %xmm13, %xmm2, %xmm1
	comultps        %xmm13, %xmm2, %xmm1
	comuleps        %xmm13, %xmm2, %xmm1
	comfalseps      %xmm13, %xmm2, %xmm1
	comuneps        %xmm13, %xmm2, %xmm1
	comunltps       %xmm13, %xmm2, %xmm1
	comunleps       %xmm13, %xmm2, %xmm1
	comtrueps       %xmm13, %xmm2, %xmm1
	comeqps         0x4(%rdx), %xmm2, %xmm1
	comltps         0x4(%rdx), %xmm2, %xmm1
	comleps         0x4(%rdx), %xmm2, %xmm1
	comunordps      0x4(%rdx), %xmm2, %xmm1
	comneps         0x4(%rdx), %xmm2, %xmm1
	comnltps        0x4(%rdx), %xmm2, %xmm1
	comnleps        0x4(%rdx), %xmm2, %xmm1
	comordps        0x4(%rdx), %xmm2, %xmm1
	comueqps        0x4(%rdx), %xmm2, %xmm1
	comultps        0x4(%rdx), %xmm2, %xmm1
	comuleps        0x4(%rdx), %xmm2, %xmm1
	comfalseps      0x4(%rdx), %xmm2, %xmm1
	comuneps        0x4(%rdx), %xmm2, %xmm1
	comunltps       0x4(%rdx), %xmm2, %xmm1
	comunleps       0x4(%rdx), %xmm2, %xmm1
	comtrueps       0x4(%rdx), %xmm2, %xmm1
	compd           $0x4, %xmm13, %xmm2, %xmm1
	compd           $0x4, 0x4(%rdx), %xmm2, %xmm1
	comeqpd         %xmm13, %xmm2, %xmm1
	comltpd         %xmm13, %xmm2, %xmm1
	comlepd         %xmm13, %xmm2, %xmm1
	comunordpd      %xmm13, %xmm2, %xmm1
	comnepd         %xmm13, %xmm2, %xmm1
	comnltpd        %xmm13, %xmm2, %xmm1
	comnlepd        %xmm13, %xmm2, %xmm1
	comordpd        %xmm13, %xmm2, %xmm1
	comueqpd        %xmm13, %xmm2, %xmm1
	comultpd        %xmm13, %xmm2, %xmm1
	comulepd        %xmm13, %xmm2, %xmm1
	comfalsepd      %xmm13, %xmm2, %xmm1
	comunepd        %xmm13, %xmm2, %xmm1
	comunltpd       %xmm13, %xmm2, %xmm1
	comunlepd       %xmm13, %xmm2, %xmm1
	comtruepd       %xmm13, %xmm2, %xmm1
	comeqpd         0x4(%rdx), %xmm2, %xmm1
	comltpd         0x4(%rdx), %xmm2, %xmm1
	comlepd         0x4(%rdx), %xmm2, %xmm1
	comunordpd      0x4(%rdx), %xmm2, %xmm1
	comnepd         0x4(%rdx), %xmm2, %xmm1
	comnltpd        0x4(%rdx), %xmm2, %xmm1
	comnlepd        0x4(%rdx), %xmm2, %xmm1
	comordpd        0x4(%rdx), %xmm2, %xmm1
	comueqpd        0x4(%rdx), %xmm2, %xmm1
	comultpd        0x4(%rdx), %xmm2, %xmm1
	comulepd        0x4(%rdx), %xmm2, %xmm1
	comfalsepd      0x4(%rdx), %xmm2, %xmm1
	comunepd        0x4(%rdx), %xmm2, %xmm1
	comunltpd       0x4(%rdx), %xmm2, %xmm1
	comunlepd       0x4(%rdx), %xmm2, %xmm1
	comtruepd       0x4(%rdx), %xmm2, %xmm1

	pcomb           $0x4, %xmm13, %xmm2, %xmm1
	pcomb           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltb         %xmm13, %xmm2, %xmm1
	pcomleb         %xmm13, %xmm2, %xmm1
	pcomgtb         %xmm13, %xmm2, %xmm1
	pcomgeb         %xmm13, %xmm2, %xmm1
	pcomeqb         %xmm13, %xmm2, %xmm1
	pcomneqb        %xmm13, %xmm2, %xmm1
	pcomltb         0x4(%rdx), %xmm2, %xmm1
	pcomleb         0x4(%rdx), %xmm2, %xmm1
	pcomgtb         0x4(%rdx), %xmm2, %xmm1
	pcomgeb         0x4(%rdx), %xmm2, %xmm1
	pcomeqb         0x4(%rdx), %xmm2, %xmm1
	pcomneqb        0x4(%rdx), %xmm2, %xmm1
	pcomw           $0x4, %xmm13, %xmm2, %xmm1
	pcomw           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltw         %xmm13, %xmm2, %xmm1
	pcomlew         %xmm13, %xmm2, %xmm1
	pcomgtw         %xmm13, %xmm2, %xmm1
	pcomgew         %xmm13, %xmm2, %xmm1
	pcomeqw         %xmm13, %xmm2, %xmm1
	pcomneqw        %xmm13, %xmm2, %xmm1
	pcomltw         0x4(%rdx), %xmm2, %xmm1
	pcomlew         0x4(%rdx), %xmm2, %xmm1
	pcomgtw         0x4(%rdx), %xmm2, %xmm1
	pcomgew         0x4(%rdx), %xmm2, %xmm1
	pcomeqw         0x4(%rdx), %xmm2, %xmm1
	pcomneqw        0x4(%rdx), %xmm2, %xmm1
	pcomd           $0x4, %xmm13, %xmm2, %xmm1
	pcomd           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltd         %xmm13, %xmm2, %xmm1
	pcomled         %xmm13, %xmm2, %xmm1
	pcomgtd         %xmm13, %xmm2, %xmm1
	pcomged         %xmm13, %xmm2, %xmm1
	pcomeqd         %xmm13, %xmm2, %xmm1
	pcomneqd        %xmm13, %xmm2, %xmm1
	pcomltd         0x4(%rdx), %xmm2, %xmm1
	pcomled         0x4(%rdx), %xmm2, %xmm1
	pcomgtd         0x4(%rdx), %xmm2, %xmm1
	pcomged         0x4(%rdx), %xmm2, %xmm1
	pcomeqd         0x4(%rdx), %xmm2, %xmm1
	pcomneqd        0x4(%rdx), %xmm2, %xmm1
	pcomq           $0x4, %xmm13, %xmm2, %xmm1
	pcomq           $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltq         %xmm13, %xmm2, %xmm1
	pcomleq         %xmm13, %xmm2, %xmm1
	pcomgtq         %xmm13, %xmm2, %xmm1
	pcomgeq         %xmm13, %xmm2, %xmm1
	pcomeqq         %xmm13, %xmm2, %xmm1
	pcomneqq        %xmm13, %xmm2, %xmm1
	pcomltq         0x4(%rdx), %xmm2, %xmm1
	pcomleq         0x4(%rdx), %xmm2, %xmm1
	pcomgtq         0x4(%rdx), %xmm2, %xmm1
	pcomgeq         0x4(%rdx), %xmm2, %xmm1
	pcomeqq         0x4(%rdx), %xmm2, %xmm1
	pcomneqq        0x4(%rdx), %xmm2, %xmm1

	pcomub          $0x4, %xmm13, %xmm2, %xmm1
	pcomub          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltub        %xmm13, %xmm2, %xmm1
	pcomleub        %xmm13, %xmm2, %xmm1
	pcomgtub        %xmm13, %xmm2, %xmm1
	pcomgeub        %xmm13, %xmm2, %xmm1
	pcomequb        %xmm13, %xmm2, %xmm1
	pcomnequb       %xmm13, %xmm2, %xmm1
	pcomltub        0x4(%rdx), %xmm2, %xmm1
	pcomleub        0x4(%rdx), %xmm2, %xmm1
	pcomgtub        0x4(%rdx), %xmm2, %xmm1
	pcomgeub        0x4(%rdx), %xmm2, %xmm1
	pcomequb        0x4(%rdx), %xmm2, %xmm1
	pcomnequb       0x4(%rdx), %xmm2, %xmm1
	pcomuw          $0x4, %xmm13, %xmm2, %xmm1
	pcomuw          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltuw        %xmm13, %xmm2, %xmm1
	pcomleuw        %xmm13, %xmm2, %xmm1
	pcomgtuw        %xmm13, %xmm2, %xmm1
	pcomgeuw        %xmm13, %xmm2, %xmm1
	pcomequw        %xmm13, %xmm2, %xmm1
	pcomnequw       %xmm13, %xmm2, %xmm1
	pcomltuw        0x4(%rdx), %xmm2, %xmm1
	pcomleuw        0x4(%rdx), %xmm2, %xmm1
	pcomgtuw        0x4(%rdx), %xmm2, %xmm1
	pcomgeuw        0x4(%rdx), %xmm2, %xmm1
	pcomequw        0x4(%rdx), %xmm2, %xmm1
	pcomnequw       0x4(%rdx), %xmm2, %xmm1
	pcomud          $0x4, %xmm13, %xmm2, %xmm1
	pcomud          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltud        %xmm13, %xmm2, %xmm1
	pcomleud        %xmm13, %xmm2, %xmm1
	pcomgtud        %xmm13, %xmm2, %xmm1
	pcomgeud        %xmm13, %xmm2, %xmm1
	pcomequd        %xmm13, %xmm2, %xmm1
	pcomnequd       %xmm13, %xmm2, %xmm1
	pcomltud        0x4(%rdx), %xmm2, %xmm1
	pcomleud        0x4(%rdx), %xmm2, %xmm1
	pcomgtud        0x4(%rdx), %xmm2, %xmm1
	pcomgeud        0x4(%rdx), %xmm2, %xmm1
	pcomequd        0x4(%rdx), %xmm2, %xmm1
	pcomnequd       0x4(%rdx), %xmm2, %xmm1
	pcomuq          $0x4, %xmm13, %xmm2, %xmm1
	pcomuq          $0x4, 0x4(%rdx), %xmm2, %xmm1
	pcomltuq        %xmm13, %xmm2, %xmm1
	pcomleuq        %xmm13, %xmm2, %xmm1
	pcomgtuq        %xmm13, %xmm2, %xmm1
	pcomgeuq        %xmm13, %xmm2, %xmm1
	pcomequq        %xmm13, %xmm2, %xmm1
	pcomnequq       %xmm13, %xmm2, %xmm1
	pcomltuq        0x4(%rdx), %xmm2, %xmm1
	pcomleuq        0x4(%rdx), %xmm2, %xmm1
	pcomgtuq        0x4(%rdx), %xmm2, %xmm1
	pcomgeuq        0x4(%rdx), %xmm2, %xmm1
	pcomequq        0x4(%rdx), %xmm2, %xmm1
	pcomnequq       0x4(%rdx), %xmm2, %xmm1

	comss           $0x4, %xmm3, %xmm2, %xmm1
	comss           $0x4, 0x100000(%r15), %xmm2, %xmm1
	comeqss         %xmm3, %xmm2, %xmm1
	comltss         %xmm3, %xmm2, %xmm1
	comless         %xmm3, %xmm2, %xmm1
	comunordss      %xmm3, %xmm2, %xmm1
	comness         %xmm3, %xmm2, %xmm1
	comnltss        %xmm3, %xmm2, %xmm1
	comnless        %xmm3, %xmm2, %xmm1
	comordss        %xmm3, %xmm2, %xmm1
	comueqss        %xmm3, %xmm2, %xmm1
	comultss        %xmm3, %xmm2, %xmm1
	comuless        %xmm3, %xmm2, %xmm1
	comfalsess      %xmm3, %xmm2, %xmm1
	comuness        %xmm3, %xmm2, %xmm1
	comunltss       %xmm3, %xmm2, %xmm1
	comunless       %xmm3, %xmm2, %xmm1
	comtruess       %xmm3, %xmm2, %xmm1
	comeqss         0x100000(%r15), %xmm2, %xmm1
	comltss         0x100000(%r15), %xmm2, %xmm1
	comless         0x100000(%r15), %xmm2, %xmm1
	comunordss      0x100000(%r15), %xmm2, %xmm1
	comness         0x100000(%r15), %xmm2, %xmm1
	comnltss        0x100000(%r15), %xmm2, %xmm1
	comnless        0x100000(%r15), %xmm2, %xmm1
	comordss        0x100000(%r15), %xmm2, %xmm1
	comueqss        0x100000(%r15), %xmm2, %xmm1
	comultss        0x100000(%r15), %xmm2, %xmm1
	comuless        0x100000(%r15), %xmm2, %xmm1
	comfalsess      0x100000(%r15), %xmm2, %xmm1
	comuness        0x100000(%r15), %xmm2, %xmm1
	comunltss       0x100000(%r15), %xmm2, %xmm1
	comunless       0x100000(%r15), %xmm2, %xmm1
	comtruess       0x100000(%r15), %xmm2, %xmm1
	comsd           $0x4, %xmm3, %xmm2, %xmm1
	comsd           $0x4, 0x100000(%r15), %xmm2, %xmm1
	comeqsd         %xmm3, %xmm2, %xmm1
	comltsd         %xmm3, %xmm2, %xmm1
	comlesd         %xmm3, %xmm2, %xmm1
	comunordsd      %xmm3, %xmm2, %xmm1
	comnesd         %xmm3, %xmm2, %xmm1
	comnltsd        %xmm3, %xmm2, %xmm1
	comnlesd        %xmm3, %xmm2, %xmm1
	comordsd        %xmm3, %xmm2, %xmm1
	comueqsd        %xmm3, %xmm2, %xmm1
	comultsd        %xmm3, %xmm2, %xmm1
	comulesd        %xmm3, %xmm2, %xmm1
	comfalsesd      %xmm3, %xmm2, %xmm1
	comunesd        %xmm3, %xmm2, %xmm1
	comunltsd       %xmm3, %xmm2, %xmm1
	comunlesd       %xmm3, %xmm2, %xmm1
	comtruesd       %xmm3, %xmm2, %xmm1
	comeqsd         0x100000(%r15), %xmm2, %xmm1
	comltsd         0x100000(%r15), %xmm2, %xmm1
	comlesd         0x100000(%r15), %xmm2, %xmm1
	comunordsd      0x100000(%r15), %xmm2, %xmm1
	comnesd         0x100000(%r15), %xmm2, %xmm1
	comnltsd        0x100000(%r15), %xmm2, %xmm1
	comnlesd        0x100000(%r15), %xmm2, %xmm1
	comordsd        0x100000(%r15), %xmm2, %xmm1
	comueqsd        0x100000(%r15), %xmm2, %xmm1
	comultsd        0x100000(%r15), %xmm2, %xmm1
	comulesd        0x100000(%r15), %xmm2, %xmm1
	comfalsesd      0x100000(%r15), %xmm2, %xmm1
	comunesd        0x100000(%r15), %xmm2, %xmm1
	comunltsd       0x100000(%r15), %xmm2, %xmm1
	comunlesd       0x100000(%r15), %xmm2, %xmm1
	comtruesd       0x100000(%r15), %xmm2, %xmm1
	comps           $0x4, %xmm3, %xmm2, %xmm1
	comps           $0x4, 0x100000(%r15), %xmm2, %xmm1
	comeqps         %xmm3, %xmm2, %xmm1
	comltps         %xmm3, %xmm2, %xmm1
	comleps         %xmm3, %xmm2, %xmm1
	comunordps      %xmm3, %xmm2, %xmm1
	comneps         %xmm3, %xmm2, %xmm1
	comnltps        %xmm3, %xmm2, %xmm1
	comnleps        %xmm3, %xmm2, %xmm1
	comordps        %xmm3, %xmm2, %xmm1
	comueqps        %xmm3, %xmm2, %xmm1
	comultps        %xmm3, %xmm2, %xmm1
	comuleps        %xmm3, %xmm2, %xmm1
	comfalseps      %xmm3, %xmm2, %xmm1
	comuneps        %xmm3, %xmm2, %xmm1
	comunltps       %xmm3, %xmm2, %xmm1
	comunleps       %xmm3, %xmm2, %xmm1
	comtrueps       %xmm3, %xmm2, %xmm1
	comeqps         0x100000(%r15), %xmm2, %xmm1
	comltps         0x100000(%r15), %xmm2, %xmm1
	comleps         0x100000(%r15), %xmm2, %xmm1
	comunordps      0x100000(%r15), %xmm2, %xmm1
	comneps         0x100000(%r15), %xmm2, %xmm1
	comnltps        0x100000(%r15), %xmm2, %xmm1
	comnleps        0x100000(%r15), %xmm2, %xmm1
	comordps        0x100000(%r15), %xmm2, %xmm1
	comueqps        0x100000(%r15), %xmm2, %xmm1
	comultps        0x100000(%r15), %xmm2, %xmm1
	comuleps        0x100000(%r15), %xmm2, %xmm1
	comfalseps      0x100000(%r15), %xmm2, %xmm1
	comuneps        0x100000(%r15), %xmm2, %xmm1
	comunltps       0x100000(%r15), %xmm2, %xmm1
	comunleps       0x100000(%r15), %xmm2, %xmm1
	comtrueps       0x100000(%r15), %xmm2, %xmm1
	compd           $0x4, %xmm3, %xmm2, %xmm1
	compd           $0x4, 0x100000(%r15), %xmm2, %xmm1
	comeqpd         %xmm3, %xmm2, %xmm1
	comltpd         %xmm3, %xmm2, %xmm1
	comlepd         %xmm3, %xmm2, %xmm1
	comunordpd      %xmm3, %xmm2, %xmm1
	comnepd         %xmm3, %xmm2, %xmm1
	comnltpd        %xmm3, %xmm2, %xmm1
	comnlepd        %xmm3, %xmm2, %xmm1
	comordpd        %xmm3, %xmm2, %xmm1
	comueqpd        %xmm3, %xmm2, %xmm1
	comultpd        %xmm3, %xmm2, %xmm1
	comulepd        %xmm3, %xmm2, %xmm1
	comfalsepd      %xmm3, %xmm2, %xmm1
	comunepd        %xmm3, %xmm2, %xmm1
	comunltpd       %xmm3, %xmm2, %xmm1
	comunlepd       %xmm3, %xmm2, %xmm1
	comtruepd       %xmm3, %xmm2, %xmm1
	comeqpd         0x100000(%r15), %xmm2, %xmm1
	comltpd         0x100000(%r15), %xmm2, %xmm1
	comlepd         0x100000(%r15), %xmm2, %xmm1
	comunordpd      0x100000(%r15), %xmm2, %xmm1
	comnepd         0x100000(%r15), %xmm2, %xmm1
	comnltpd        0x100000(%r15), %xmm2, %xmm1
	comnlepd        0x100000(%r15), %xmm2, %xmm1
	comordpd        0x100000(%r15), %xmm2, %xmm1
	comueqpd        0x100000(%r15), %xmm2, %xmm1
	comultpd        0x100000(%r15), %xmm2, %xmm1
	comulepd        0x100000(%r15), %xmm2, %xmm1
	comfalsepd      0x100000(%r15), %xmm2, %xmm1
	comunepd        0x100000(%r15), %xmm2, %xmm1
	comunltpd       0x100000(%r15), %xmm2, %xmm1
	comunlepd       0x100000(%r15), %xmm2, %xmm1
	comtruepd       0x100000(%r15), %xmm2, %xmm1

	pcomb           $0x4, %xmm3, %xmm2, %xmm1
	pcomb           $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltb         %xmm3, %xmm2, %xmm1
	pcomleb         %xmm3, %xmm2, %xmm1
	pcomgtb         %xmm3, %xmm2, %xmm1
	pcomgeb         %xmm3, %xmm2, %xmm1
	pcomeqb         %xmm3, %xmm2, %xmm1
	pcomneqb        %xmm3, %xmm2, %xmm1
	pcomltb         0x100000(%r15), %xmm2, %xmm1
	pcomleb         0x100000(%r15), %xmm2, %xmm1
	pcomgtb         0x100000(%r15), %xmm2, %xmm1
	pcomgeb         0x100000(%r15), %xmm2, %xmm1
	pcomeqb         0x100000(%r15), %xmm2, %xmm1
	pcomneqb        0x100000(%r15), %xmm2, %xmm1
	pcomw           $0x4, %xmm3, %xmm2, %xmm1
	pcomw           $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltw         %xmm3, %xmm2, %xmm1
	pcomlew         %xmm3, %xmm2, %xmm1
	pcomgtw         %xmm3, %xmm2, %xmm1
	pcomgew         %xmm3, %xmm2, %xmm1
	pcomeqw         %xmm3, %xmm2, %xmm1
	pcomneqw        %xmm3, %xmm2, %xmm1
	pcomltw         0x100000(%r15), %xmm2, %xmm1
	pcomlew         0x100000(%r15), %xmm2, %xmm1
	pcomgtw         0x100000(%r15), %xmm2, %xmm1
	pcomgew         0x100000(%r15), %xmm2, %xmm1
	pcomeqw         0x100000(%r15), %xmm2, %xmm1
	pcomneqw        0x100000(%r15), %xmm2, %xmm1
	pcomd           $0x4, %xmm3, %xmm2, %xmm1
	pcomd           $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltd         %xmm3, %xmm2, %xmm1
	pcomled         %xmm3, %xmm2, %xmm1
	pcomgtd         %xmm3, %xmm2, %xmm1
	pcomged         %xmm3, %xmm2, %xmm1
	pcomeqd         %xmm3, %xmm2, %xmm1
	pcomneqd        %xmm3, %xmm2, %xmm1
	pcomltd         0x100000(%r15), %xmm2, %xmm1
	pcomled         0x100000(%r15), %xmm2, %xmm1
	pcomgtd         0x100000(%r15), %xmm2, %xmm1
	pcomged         0x100000(%r15), %xmm2, %xmm1
	pcomeqd         0x100000(%r15), %xmm2, %xmm1
	pcomneqd        0x100000(%r15), %xmm2, %xmm1
	pcomq           $0x4, %xmm3, %xmm2, %xmm1
	pcomq           $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltq         %xmm3, %xmm2, %xmm1
	pcomleq         %xmm3, %xmm2, %xmm1
	pcomgtq         %xmm3, %xmm2, %xmm1
	pcomgeq         %xmm3, %xmm2, %xmm1
	pcomeqq         %xmm3, %xmm2, %xmm1
	pcomneqq        %xmm3, %xmm2, %xmm1
	pcomltq         0x100000(%r15), %xmm2, %xmm1
	pcomleq         0x100000(%r15), %xmm2, %xmm1
	pcomgtq         0x100000(%r15), %xmm2, %xmm1
	pcomgeq         0x100000(%r15), %xmm2, %xmm1
	pcomeqq         0x100000(%r15), %xmm2, %xmm1
	pcomneqq        0x100000(%r15), %xmm2, %xmm1

	pcomub          $0x4, %xmm3, %xmm2, %xmm1
	pcomub          $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltub        %xmm3, %xmm2, %xmm1
	pcomleub        %xmm3, %xmm2, %xmm1
	pcomgtub        %xmm3, %xmm2, %xmm1
	pcomgeub        %xmm3, %xmm2, %xmm1
	pcomequb        %xmm3, %xmm2, %xmm1
	pcomnequb       %xmm3, %xmm2, %xmm1
	pcomltub        0x100000(%r15), %xmm2, %xmm1
	pcomleub        0x100000(%r15), %xmm2, %xmm1
	pcomgtub        0x100000(%r15), %xmm2, %xmm1
	pcomgeub        0x100000(%r15), %xmm2, %xmm1
	pcomequb        0x100000(%r15), %xmm2, %xmm1
	pcomnequb       0x100000(%r15), %xmm2, %xmm1
	pcomuw          $0x4, %xmm3, %xmm2, %xmm1
	pcomuw          $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltuw        %xmm3, %xmm2, %xmm1
	pcomleuw        %xmm3, %xmm2, %xmm1
	pcomgtuw        %xmm3, %xmm2, %xmm1
	pcomgeuw        %xmm3, %xmm2, %xmm1
	pcomequw        %xmm3, %xmm2, %xmm1
	pcomnequw       %xmm3, %xmm2, %xmm1
	pcomltuw        0x100000(%r15), %xmm2, %xmm1
	pcomleuw        0x100000(%r15), %xmm2, %xmm1
	pcomgtuw        0x100000(%r15), %xmm2, %xmm1
	pcomgeuw        0x100000(%r15), %xmm2, %xmm1
	pcomequw        0x100000(%r15), %xmm2, %xmm1
	pcomnequw       0x100000(%r15), %xmm2, %xmm1
	pcomud          $0x4, %xmm3, %xmm2, %xmm1
	pcomud          $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltud        %xmm3, %xmm2, %xmm1
	pcomleud        %xmm3, %xmm2, %xmm1
	pcomgtud        %xmm3, %xmm2, %xmm1
	pcomgeud        %xmm3, %xmm2, %xmm1
	pcomequd        %xmm3, %xmm2, %xmm1
	pcomnequd       %xmm3, %xmm2, %xmm1
	pcomltud        0x100000(%r15), %xmm2, %xmm1
	pcomleud        0x100000(%r15), %xmm2, %xmm1
	pcomgtud        0x100000(%r15), %xmm2, %xmm1
	pcomgeud        0x100000(%r15), %xmm2, %xmm1
	pcomequd        0x100000(%r15), %xmm2, %xmm1
	pcomnequd       0x100000(%r15), %xmm2, %xmm1
	pcomuq          $0x4, %xmm3, %xmm2, %xmm1
	pcomuq          $0x4, 0x100000(%r15), %xmm2, %xmm1
	pcomltuq        %xmm3, %xmm2, %xmm1
	pcomleuq        %xmm3, %xmm2, %xmm1
	pcomgtuq        %xmm3, %xmm2, %xmm1
	pcomgeuq        %xmm3, %xmm2, %xmm1
	pcomequq        %xmm3, %xmm2, %xmm1
	pcomnequq       %xmm3, %xmm2, %xmm1
	pcomltuq        0x100000(%r15), %xmm2, %xmm1
	pcomleuq        0x100000(%r15), %xmm2, %xmm1
	pcomgtuq        0x100000(%r15), %xmm2, %xmm1
	pcomgeuq        0x100000(%r15), %xmm2, %xmm1
	pcomequq        0x100000(%r15), %xmm2, %xmm1
	pcomnequq       0x100000(%r15), %xmm2, %xmm1

	frczss          %xmm2, %xmm1
	frczss          0x4(%rdx), %xmm1
	frczsd          %xmm2, %xmm1
	frczsd          0x4(%rdx), %xmm1
	frczps          %xmm2, %xmm1
	frczps          0x4(%rdx), %xmm1
	frczpd          %xmm2, %xmm1
	frczpd          0x4(%rdx), %xmm1

	frczss          %xmm12, %xmm11
	frczss          0x100000(%r15), %xmm11
	frczsd          %xmm12, %xmm11
	frczsd          0x100000(%r15), %xmm11
	frczps          %xmm12, %xmm11
	frczps          0x100000(%r15), %xmm11
	frczpd          %xmm12, %xmm11
	frczpd          0x100000(%r15), %xmm11

	frczss          %xmm12, %xmm1
	frczss          0x4(%rdx), %xmm1
	frczsd          %xmm12, %xmm1
	frczsd          0x4(%rdx), %xmm1
	frczps          %xmm12, %xmm1
	frczps          0x4(%rdx), %xmm1
	frczpd          %xmm12, %xmm1
	frczpd          0x4(%rdx), %xmm1

	frczss          %xmm2, %xmm11
	frczss          0x4(%rdx), %xmm11
	frczsd          %xmm2, %xmm11
	frczsd          0x4(%rdx), %xmm11
	frczps          %xmm2, %xmm11
	frczps          0x4(%rdx), %xmm11
	frczpd          %xmm2, %xmm11
	frczpd          0x4(%rdx), %xmm11

	frczss          %xmm2, %xmm1
	frczss          0x4(%rdx), %xmm1
	frczsd          %xmm2, %xmm1
	frczsd          0x4(%rdx), %xmm1
	frczps          %xmm2, %xmm1
	frczps          0x4(%rdx), %xmm1
	frczpd          %xmm2, %xmm1
	frczpd          0x4(%rdx), %xmm1

	frczss          %xmm2, %xmm1
	frczss          0x100000(%r15), %xmm1
	frczsd          %xmm2, %xmm1
	frczsd          0x100000(%r15), %xmm1
	frczps          %xmm2, %xmm1
	frczps          0x100000(%r15), %xmm1
	frczpd          %xmm2, %xmm1
	frczpd          0x100000(%r15), %xmm1

	cvtph2ps        %xmm2, %xmm1
	cvtph2ps        0x4(%rdx), %xmm1
	cvtps2ph        %xmm2, %xmm1
	cvtps2ph        %xmm1, 0x4(%rdx)

	cvtph2ps        %xmm12, %xmm11
	cvtph2ps        0x100000(%r15), %xmm11
	cvtps2ph        %xmm12, %xmm11
	cvtps2ph        %xmm11, 0x100000(%r15)

	cvtph2ps        %xmm12, %xmm1
	cvtph2ps        0x4(%rdx), %xmm1
	cvtps2ph        %xmm12, %xmm1
	cvtps2ph        %xmm1, 0x4(%rdx)

	cvtph2ps        %xmm2, %xmm11
	cvtph2ps        0x4(%rdx), %xmm11
	cvtps2ph        %xmm2, %xmm11
	cvtps2ph        %xmm11, 0x4(%rdx)

	cvtph2ps        %xmm2, %xmm1
	cvtph2ps        0x4(%rdx), %xmm1
	cvtps2ph        %xmm2, %xmm1
	cvtps2ph        %xmm1, 0x4(%rdx)

	cvtph2ps        %xmm2, %xmm1
	cvtph2ps        0x100000(%r15), %xmm1
	cvtps2ph        %xmm2, %xmm1
	cvtps2ph        %xmm1, 0x100000(%r15)

	protb           $0x4, 0x4(%rdx), %xmm1
	protw		$0x1, 0x8(%r14), %xmm2
	protq		$0x3, 0x4(%r15), %xmm1
	
	ret
	.size	foo, .-foo
