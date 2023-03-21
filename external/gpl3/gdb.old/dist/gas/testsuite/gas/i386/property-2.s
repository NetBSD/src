	.text
	fsin
	movq		%mm0, %mm1
	fxsave		(%eax)
	xsave		(%eax)
	xsaveopt	(%eax)
	xsavec		(%eax)
	cmove		%eax,%ebx
	movaps		%xmm0, %xmm1
	movapd		%xmm0, %xmm1
	mwait
	psignb		%xmm0, %xmm1
	blendvpd	%xmm0, %xmm1
	pcmpgtq		%xmm0, %xmm1
	vmovaps		%xmm0, %xmm1
	vpabsb		%ymm0, %ymm1
	vfmadd231ps	%ymm0, %ymm1, %ymm1
	vmovaps		%zmm0, %zmm1
	vplzcntd	%zmm0, %zmm1
	vrsqrt28pd	%zmm0, %zmm1
	vscatterpf0dpd	(%eax,%ymm1){%k1}
	{evex} vpmovzxdq %xmm0, %xmm1
	vandnpd		%zmm0, %zmm0, %zmm1
	vpmaxuw		%zmm0, %zmm0, %zmm1
	v4fnmaddss	(%ecx), %xmm4, %xmm1
	vpopcntb	%zmm0, %zmm1
	vp4dpwssd	(%ecx), %zmm0, %zmm1
	vpmadd52luq	(%ecx), %zmm0, %zmm1
	vpermt2b	(%ecx), %zmm0, %zmm1
	vpcompressb	%zmm0, %zmm1
	vpdpwssds	(%ecx), %zmm0, %zmm1
	vcvtne2ps2bf16	(%ecx), %zmm0, %zmm1
