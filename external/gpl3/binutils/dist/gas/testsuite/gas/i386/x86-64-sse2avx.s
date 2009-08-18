# Check 64bit SSE to AVX instructions

	.allow_index_reg
	.text
_start:
# Tests for op mem64
	ldmxcsr (%rcx)
	stmxcsr (%rcx)

# Tests for op xmm/mem128, xmm
	cvtdq2ps %xmm4,%xmm6
	cvtdq2ps (%rcx),%xmm4
	cvtpd2dq %xmm4,%xmm6
	cvtpd2dq (%rcx),%xmm4
	cvtpd2ps %xmm4,%xmm6
	cvtpd2ps (%rcx),%xmm4
	cvtps2dq %xmm4,%xmm6
	cvtps2dq (%rcx),%xmm4
	cvttpd2dq %xmm4,%xmm6
	cvttpd2dq (%rcx),%xmm4
	cvttps2dq %xmm4,%xmm6
	cvttps2dq (%rcx),%xmm4
	movapd %xmm4,%xmm6
	movapd (%rcx),%xmm4
	movaps %xmm4,%xmm6
	movaps (%rcx),%xmm4
	movdqa %xmm4,%xmm6
	movdqa (%rcx),%xmm4
	movdqu %xmm4,%xmm6
	movdqu (%rcx),%xmm4
	movshdup %xmm4,%xmm6
	movshdup (%rcx),%xmm4
	movsldup %xmm4,%xmm6
	movsldup (%rcx),%xmm4
	movupd %xmm4,%xmm6
	movupd (%rcx),%xmm4
	movups %xmm4,%xmm6
	movups (%rcx),%xmm4
	pabsb %xmm4,%xmm6
	pabsb (%rcx),%xmm4
	pabsw %xmm4,%xmm6
	pabsw (%rcx),%xmm4
	pabsd %xmm4,%xmm6
	pabsd (%rcx),%xmm4
	phminposuw %xmm4,%xmm6
	phminposuw (%rcx),%xmm4
	ptest %xmm4,%xmm6
	ptest (%rcx),%xmm4
	rcpps %xmm4,%xmm6
	rcpps (%rcx),%xmm4
	rsqrtps %xmm4,%xmm6
	rsqrtps (%rcx),%xmm4
	sqrtpd %xmm4,%xmm6
	sqrtpd (%rcx),%xmm4
	sqrtps %xmm4,%xmm6
	sqrtps (%rcx),%xmm4
	aesimc %xmm4,%xmm6
	aesimc (%rcx),%xmm4

# Tests for op xmm, xmm/mem128
	movapd %xmm4,%xmm6
	movapd %xmm4,(%rcx)
	movaps %xmm4,%xmm6
	movaps %xmm4,(%rcx)
	movdqa %xmm4,%xmm6
	movdqa %xmm4,(%rcx)
	movdqu %xmm4,%xmm6
	movdqu %xmm4,(%rcx)
	movupd %xmm4,%xmm6
	movupd %xmm4,(%rcx)
	movups %xmm4,%xmm6
	movups %xmm4,(%rcx)

# Tests for op mem128, xmm
	lddqu (%rcx),%xmm4
	movntdqa (%rcx),%xmm4

# Tests for op xmm, mem128
	movntdq %xmm4,(%rcx)
	movntpd %xmm4,(%rcx)
	movntps %xmm4,(%rcx)

# Tests for op xmm/mem128, xmm[, xmm]
	addpd %xmm4,%xmm6
	addpd (%rcx),%xmm6
	addps %xmm4,%xmm6
	addps (%rcx),%xmm6
	addsubpd %xmm4,%xmm6
	addsubpd (%rcx),%xmm6
	addsubps %xmm4,%xmm6
	addsubps (%rcx),%xmm6
	andnpd %xmm4,%xmm6
	andnpd (%rcx),%xmm6
	andnps %xmm4,%xmm6
	andnps (%rcx),%xmm6
	andpd %xmm4,%xmm6
	andpd (%rcx),%xmm6
	andps %xmm4,%xmm6
	andps (%rcx),%xmm6
	divpd %xmm4,%xmm6
	divpd (%rcx),%xmm6
	divps %xmm4,%xmm6
	divps (%rcx),%xmm6
	haddpd %xmm4,%xmm6
	haddpd (%rcx),%xmm6
	haddps %xmm4,%xmm6
	haddps (%rcx),%xmm6
	hsubpd %xmm4,%xmm6
	hsubpd (%rcx),%xmm6
	hsubps %xmm4,%xmm6
	hsubps (%rcx),%xmm6
	maxpd %xmm4,%xmm6
	maxpd (%rcx),%xmm6
	maxps %xmm4,%xmm6
	maxps (%rcx),%xmm6
	minpd %xmm4,%xmm6
	minpd (%rcx),%xmm6
	minps %xmm4,%xmm6
	minps (%rcx),%xmm6
	mulpd %xmm4,%xmm6
	mulpd (%rcx),%xmm6
	mulps %xmm4,%xmm6
	mulps (%rcx),%xmm6
	orpd %xmm4,%xmm6
	orpd (%rcx),%xmm6
	orps %xmm4,%xmm6
	orps (%rcx),%xmm6
	packsswb %xmm4,%xmm6
	packsswb (%rcx),%xmm6
	packssdw %xmm4,%xmm6
	packssdw (%rcx),%xmm6
	packuswb %xmm4,%xmm6
	packuswb (%rcx),%xmm6
	packusdw %xmm4,%xmm6
	packusdw (%rcx),%xmm6
	paddb %xmm4,%xmm6
	paddb (%rcx),%xmm6
	paddw %xmm4,%xmm6
	paddw (%rcx),%xmm6
	paddd %xmm4,%xmm6
	paddd (%rcx),%xmm6
	paddq %xmm4,%xmm6
	paddq (%rcx),%xmm6
	paddsb %xmm4,%xmm6
	paddsb (%rcx),%xmm6
	paddsw %xmm4,%xmm6
	paddsw (%rcx),%xmm6
	paddusb %xmm4,%xmm6
	paddusb (%rcx),%xmm6
	paddusw %xmm4,%xmm6
	paddusw (%rcx),%xmm6
	pand %xmm4,%xmm6
	pand (%rcx),%xmm6
	pandn %xmm4,%xmm6
	pandn (%rcx),%xmm6
	pavgb %xmm4,%xmm6
	pavgb (%rcx),%xmm6
	pavgw %xmm4,%xmm6
	pavgw (%rcx),%xmm6
	pclmullqlqdq %xmm4,%xmm6
	pclmullqlqdq (%rcx),%xmm6
	pclmulhqlqdq %xmm4,%xmm6
	pclmulhqlqdq (%rcx),%xmm6
	pclmullqhqdq %xmm4,%xmm6
	pclmullqhqdq (%rcx),%xmm6
	pclmulhqhqdq %xmm4,%xmm6
	pclmulhqhqdq (%rcx),%xmm6
	pcmpeqb %xmm4,%xmm6
	pcmpeqb (%rcx),%xmm6
	pcmpeqw %xmm4,%xmm6
	pcmpeqw (%rcx),%xmm6
	pcmpeqd %xmm4,%xmm6
	pcmpeqd (%rcx),%xmm6
	pcmpeqq %xmm4,%xmm6
	pcmpeqq (%rcx),%xmm6
	pcmpgtb %xmm4,%xmm6
	pcmpgtb (%rcx),%xmm6
	pcmpgtw %xmm4,%xmm6
	pcmpgtw (%rcx),%xmm6
	pcmpgtd %xmm4,%xmm6
	pcmpgtd (%rcx),%xmm6
	pcmpgtq %xmm4,%xmm6
	pcmpgtq (%rcx),%xmm6
	phaddw %xmm4,%xmm6
	phaddw (%rcx),%xmm6
	phaddd %xmm4,%xmm6
	phaddd (%rcx),%xmm6
	phaddsw %xmm4,%xmm6
	phaddsw (%rcx),%xmm6
	phsubw %xmm4,%xmm6
	phsubw (%rcx),%xmm6
	phsubd %xmm4,%xmm6
	phsubd (%rcx),%xmm6
	phsubsw %xmm4,%xmm6
	phsubsw (%rcx),%xmm6
	pmaddwd %xmm4,%xmm6
	pmaddwd (%rcx),%xmm6
	pmaddubsw %xmm4,%xmm6
	pmaddubsw (%rcx),%xmm6
	pmaxsb %xmm4,%xmm6
	pmaxsb (%rcx),%xmm6
	pmaxsw %xmm4,%xmm6
	pmaxsw (%rcx),%xmm6
	pmaxsd %xmm4,%xmm6
	pmaxsd (%rcx),%xmm6
	pmaxub %xmm4,%xmm6
	pmaxub (%rcx),%xmm6
	pmaxuw %xmm4,%xmm6
	pmaxuw (%rcx),%xmm6
	pmaxud %xmm4,%xmm6
	pmaxud (%rcx),%xmm6
	pminsb %xmm4,%xmm6
	pminsb (%rcx),%xmm6
	pminsw %xmm4,%xmm6
	pminsw (%rcx),%xmm6
	pminsd %xmm4,%xmm6
	pminsd (%rcx),%xmm6
	pminub %xmm4,%xmm6
	pminub (%rcx),%xmm6
	pminuw %xmm4,%xmm6
	pminuw (%rcx),%xmm6
	pminud %xmm4,%xmm6
	pminud (%rcx),%xmm6
	pmulhuw %xmm4,%xmm6
	pmulhuw (%rcx),%xmm6
	pmulhrsw %xmm4,%xmm6
	pmulhrsw (%rcx),%xmm6
	pmulhw %xmm4,%xmm6
	pmulhw (%rcx),%xmm6
	pmullw %xmm4,%xmm6
	pmullw (%rcx),%xmm6
	pmulld %xmm4,%xmm6
	pmulld (%rcx),%xmm6
	pmuludq %xmm4,%xmm6
	pmuludq (%rcx),%xmm6
	pmuldq %xmm4,%xmm6
	pmuldq (%rcx),%xmm6
	por %xmm4,%xmm6
	por (%rcx),%xmm6
	psadbw %xmm4,%xmm6
	psadbw (%rcx),%xmm6
	pshufb %xmm4,%xmm6
	pshufb (%rcx),%xmm6
	psignb %xmm4,%xmm6
	psignb (%rcx),%xmm6
	psignw %xmm4,%xmm6
	psignw (%rcx),%xmm6
	psignd %xmm4,%xmm6
	psignd (%rcx),%xmm6
	psllw %xmm4,%xmm6
	psllw (%rcx),%xmm6
	pslld %xmm4,%xmm6
	pslld (%rcx),%xmm6
	psllq %xmm4,%xmm6
	psllq (%rcx),%xmm6
	psraw %xmm4,%xmm6
	psraw (%rcx),%xmm6
	psrad %xmm4,%xmm6
	psrad (%rcx),%xmm6
	psrlw %xmm4,%xmm6
	psrlw (%rcx),%xmm6
	psrld %xmm4,%xmm6
	psrld (%rcx),%xmm6
	psrlq %xmm4,%xmm6
	psrlq (%rcx),%xmm6
	psubb %xmm4,%xmm6
	psubb (%rcx),%xmm6
	psubw %xmm4,%xmm6
	psubw (%rcx),%xmm6
	psubd %xmm4,%xmm6
	psubd (%rcx),%xmm6
	psubq %xmm4,%xmm6
	psubq (%rcx),%xmm6
	psubsb %xmm4,%xmm6
	psubsb (%rcx),%xmm6
	psubsw %xmm4,%xmm6
	psubsw (%rcx),%xmm6
	psubusb %xmm4,%xmm6
	psubusb (%rcx),%xmm6
	psubusw %xmm4,%xmm6
	psubusw (%rcx),%xmm6
	punpckhbw %xmm4,%xmm6
	punpckhbw (%rcx),%xmm6
	punpckhwd %xmm4,%xmm6
	punpckhwd (%rcx),%xmm6
	punpckhdq %xmm4,%xmm6
	punpckhdq (%rcx),%xmm6
	punpckhqdq %xmm4,%xmm6
	punpckhqdq (%rcx),%xmm6
	punpcklbw %xmm4,%xmm6
	punpcklbw (%rcx),%xmm6
	punpcklwd %xmm4,%xmm6
	punpcklwd (%rcx),%xmm6
	punpckldq %xmm4,%xmm6
	punpckldq (%rcx),%xmm6
	punpcklqdq %xmm4,%xmm6
	punpcklqdq (%rcx),%xmm6
	pxor %xmm4,%xmm6
	pxor (%rcx),%xmm6
	subpd %xmm4,%xmm6
	subpd (%rcx),%xmm6
	subps %xmm4,%xmm6
	subps (%rcx),%xmm6
	unpckhpd %xmm4,%xmm6
	unpckhpd (%rcx),%xmm6
	unpckhps %xmm4,%xmm6
	unpckhps (%rcx),%xmm6
	unpcklpd %xmm4,%xmm6
	unpcklpd (%rcx),%xmm6
	unpcklps %xmm4,%xmm6
	unpcklps (%rcx),%xmm6
	xorpd %xmm4,%xmm6
	xorpd (%rcx),%xmm6
	xorps %xmm4,%xmm6
	xorps (%rcx),%xmm6
	aesenc %xmm4,%xmm6
	aesenc (%rcx),%xmm6
	aesenclast %xmm4,%xmm6
	aesenclast (%rcx),%xmm6
	aesdec %xmm4,%xmm6
	aesdec (%rcx),%xmm6
	aesdeclast %xmm4,%xmm6
	aesdeclast (%rcx),%xmm6
	cmpeqpd %xmm4,%xmm6
	cmpeqpd (%rcx),%xmm6
	cmpeqps %xmm4,%xmm6
	cmpeqps (%rcx),%xmm6
	cmpltpd %xmm4,%xmm6
	cmpltpd (%rcx),%xmm6
	cmpltps %xmm4,%xmm6
	cmpltps (%rcx),%xmm6
	cmplepd %xmm4,%xmm6
	cmplepd (%rcx),%xmm6
	cmpleps %xmm4,%xmm6
	cmpleps (%rcx),%xmm6
	cmpunordpd %xmm4,%xmm6
	cmpunordpd (%rcx),%xmm6
	cmpunordps %xmm4,%xmm6
	cmpunordps (%rcx),%xmm6
	cmpneqpd %xmm4,%xmm6
	cmpneqpd (%rcx),%xmm6
	cmpneqps %xmm4,%xmm6
	cmpneqps (%rcx),%xmm6
	cmpnltpd %xmm4,%xmm6
	cmpnltpd (%rcx),%xmm6
	cmpnltps %xmm4,%xmm6
	cmpnltps (%rcx),%xmm6
	cmpnlepd %xmm4,%xmm6
	cmpnlepd (%rcx),%xmm6
	cmpnleps %xmm4,%xmm6
	cmpnleps (%rcx),%xmm6
	cmpordpd %xmm4,%xmm6
	cmpordpd (%rcx),%xmm6
	cmpordps %xmm4,%xmm6
	cmpordps (%rcx),%xmm6

# Tests for op imm8, xmm/mem128, xmm
	aeskeygenassist $100,%xmm4,%xmm6
	aeskeygenassist $100,(%rcx),%xmm6
	pcmpestri $100,%xmm4,%xmm6
	pcmpestri $100,(%rcx),%xmm6
	pcmpestrm $100,%xmm4,%xmm6
	pcmpestrm $100,(%rcx),%xmm6
	pcmpistri $100,%xmm4,%xmm6
	pcmpistri $100,(%rcx),%xmm6
	pcmpistrm $100,%xmm4,%xmm6
	pcmpistrm $100,(%rcx),%xmm6
	pshufd $100,%xmm4,%xmm6
	pshufd $100,(%rcx),%xmm6
	pshufhw $100,%xmm4,%xmm6
	pshufhw $100,(%rcx),%xmm6
	pshuflw $100,%xmm4,%xmm6
	pshuflw $100,(%rcx),%xmm6
	roundpd $100,%xmm4,%xmm6
	roundpd $100,(%rcx),%xmm6
	roundps $100,%xmm4,%xmm6
	roundps $100,(%rcx),%xmm6

# Tests for op imm8, xmm/mem128, xmm[, xmm]
	blendpd $100,%xmm4,%xmm6
	blendpd $100,(%rcx),%xmm6
	blendps $100,%xmm4,%xmm6
	blendps $100,(%rcx),%xmm6
	cmppd $100,%xmm4,%xmm6
	cmppd $100,(%rcx),%xmm6
	cmpps $100,%xmm4,%xmm6
	cmpps $100,(%rcx),%xmm6
	dppd $100,%xmm4,%xmm6
	dppd $100,(%rcx),%xmm6
	dpps $100,%xmm4,%xmm6
	dpps $100,(%rcx),%xmm6
	mpsadbw $100,%xmm4,%xmm6
	mpsadbw $100,(%rcx),%xmm6
	palignr $100,%xmm4,%xmm6
	palignr $100,(%rcx),%xmm6
	pblendw $100,%xmm4,%xmm6
	pblendw $100,(%rcx),%xmm6
	shufpd $100,%xmm4,%xmm6
	shufpd $100,(%rcx),%xmm6
	shufps $100,%xmm4,%xmm6
	shufps $100,(%rcx),%xmm6

# Tests for op xmm0, xmm/mem128, xmm[, xmm]
	blendvpd %xmm0,%xmm4,%xmm6
	blendvpd %xmm0,(%rcx),%xmm6
	blendvpd %xmm4,%xmm6
	blendvpd (%rcx),%xmm6
	blendvps %xmm0,%xmm4,%xmm6
	blendvps %xmm0,(%rcx),%xmm6
	blendvps %xmm4,%xmm6
	blendvps (%rcx),%xmm6
	pblendvb %xmm0,%xmm4,%xmm6
	pblendvb %xmm0,(%rcx),%xmm6
	pblendvb %xmm4,%xmm6
	pblendvb (%rcx),%xmm6

# Tests for op xmm/mem64, xmm
	comisd %xmm4,%xmm6
	comisd (%rcx),%xmm4
	cvtdq2pd %xmm4,%xmm6
	cvtdq2pd (%rcx),%xmm4
	cvtps2pd %xmm4,%xmm6
	cvtps2pd (%rcx),%xmm4
	movddup %xmm4,%xmm6
	movddup (%rcx),%xmm4
	pmovsxbw %xmm4,%xmm6
	pmovsxbw (%rcx),%xmm4
	pmovsxwd %xmm4,%xmm6
	pmovsxwd (%rcx),%xmm4
	pmovsxdq %xmm4,%xmm6
	pmovsxdq (%rcx),%xmm4
	pmovzxbw %xmm4,%xmm6
	pmovzxbw (%rcx),%xmm4
	pmovzxwd %xmm4,%xmm6
	pmovzxwd (%rcx),%xmm4
	pmovzxdq %xmm4,%xmm6
	pmovzxdq (%rcx),%xmm4
	ucomisd %xmm4,%xmm6
	ucomisd (%rcx),%xmm4

# Tests for op mem64, xmm
	movsd (%rcx),%xmm4

# Tests for op xmm, mem64
	movlpd %xmm4,(%rcx)
	movlps %xmm4,(%rcx)
	movhpd %xmm4,(%rcx)
	movhps %xmm4,(%rcx)
	movsd %xmm4,(%rcx)

# Tests for op xmm, regq/mem64
# Tests for op regq/mem64, xmm
	movd %xmm4,%rcx
	movd %rcx,%xmm4
	movq %xmm4,%rcx
	movq %rcx,%xmm4
	movq %xmm4,(%rcx)
	movq (%rcx),%xmm4

# Tests for op xmm/mem64, regl
	cvtsd2si %xmm4,%ecx
	cvtsd2si (%rcx),%ecx
	cvttsd2si %xmm4,%ecx
	cvttsd2si (%rcx),%ecx

# Tests for op xmm/mem64, regq
	cvtsd2si %xmm4,%rcx
	cvtsd2si (%rcx),%rcx
	cvttsd2si %xmm4,%rcx
	cvttsd2si (%rcx),%rcx

# Tests for op regq/mem64, xmm[, xmm]
	cvtsi2sdq %rcx,%xmm4
	cvtsi2sdq (%rcx),%xmm4
	cvtsi2ssq %rcx,%xmm4
	cvtsi2ssq (%rcx),%xmm4

# Tests for op imm8, regq/mem64, xmm[, xmm]
	pinsrq $100,%rcx,%xmm4
	pinsrq $100,(%rcx),%xmm4

# Testsf for op imm8, xmm, regq/mem64
	pextrq $100,%xmm4,%rcx
	pextrq $100,%xmm4,(%rcx)

# Tests for op mem64, xmm[, xmm]
	movlpd (%rcx),%xmm4
	movlps (%rcx),%xmm4
	movhpd (%rcx),%xmm4
	movhps (%rcx),%xmm4

# Tests for op imm8, xmm/mem64, xmm[, xmm]
	cmpsd $100,%xmm4,%xmm6
	cmpsd $100,(%rcx),%xmm6
	roundsd $100,%xmm4,%xmm6
	roundsd $100,(%rcx),%xmm6

# Tests for op xmm/mem64, xmm[, xmm]
	addsd %xmm4,%xmm6
	addsd (%rcx),%xmm6
	cvtsd2ss %xmm4,%xmm6
	cvtsd2ss (%rcx),%xmm6
	divsd %xmm4,%xmm6
	divsd (%rcx),%xmm6
	maxsd %xmm4,%xmm6
	maxsd (%rcx),%xmm6
	minsd %xmm4,%xmm6
	minsd (%rcx),%xmm6
	mulsd %xmm4,%xmm6
	mulsd (%rcx),%xmm6
	sqrtsd %xmm4,%xmm6
	sqrtsd (%rcx),%xmm6
	subsd %xmm4,%xmm6
	subsd (%rcx),%xmm6
	cmpeqsd %xmm4,%xmm6
	cmpeqsd (%rcx),%xmm6
	cmpltsd %xmm4,%xmm6
	cmpltsd (%rcx),%xmm6
	cmplesd %xmm4,%xmm6
	cmplesd (%rcx),%xmm6
	cmpunordsd %xmm4,%xmm6
	cmpunordsd (%rcx),%xmm6
	cmpneqsd %xmm4,%xmm6
	cmpneqsd (%rcx),%xmm6
	cmpnltsd %xmm4,%xmm6
	cmpnltsd (%rcx),%xmm6
	cmpnlesd %xmm4,%xmm6
	cmpnlesd (%rcx),%xmm6
	cmpordsd %xmm4,%xmm6
	cmpordsd (%rcx),%xmm6

# Tests for op xmm/mem32, xmm[, xmm]
	addss %xmm4,%xmm6
	addss (%rcx),%xmm6
	cvtss2sd %xmm4,%xmm6
	cvtss2sd (%rcx),%xmm6
	divss %xmm4,%xmm6
	divss (%rcx),%xmm6
	maxss %xmm4,%xmm6
	maxss (%rcx),%xmm6
	minss %xmm4,%xmm6
	minss (%rcx),%xmm6
	mulss %xmm4,%xmm6
	mulss (%rcx),%xmm6
	rcpss %xmm4,%xmm6
	rcpss (%rcx),%xmm6
	rsqrtss %xmm4,%xmm6
	rsqrtss (%rcx),%xmm6
	sqrtss %xmm4,%xmm6
	sqrtss (%rcx),%xmm6
	subss %xmm4,%xmm6
	subss (%rcx),%xmm6
	cmpeqss %xmm4,%xmm6
	cmpeqss (%rcx),%xmm6
	cmpltss %xmm4,%xmm6
	cmpltss (%rcx),%xmm6
	cmpless %xmm4,%xmm6
	cmpless (%rcx),%xmm6
	cmpunordss %xmm4,%xmm6
	cmpunordss (%rcx),%xmm6
	cmpneqss %xmm4,%xmm6
	cmpneqss (%rcx),%xmm6
	cmpnltss %xmm4,%xmm6
	cmpnltss (%rcx),%xmm6
	cmpnless %xmm4,%xmm6
	cmpnless (%rcx),%xmm6
	cmpordss %xmm4,%xmm6
	cmpordss (%rcx),%xmm6

# Tests for op xmm/mem32, xmm
	comiss %xmm4,%xmm6
	comiss (%rcx),%xmm4
	pmovsxbd %xmm4,%xmm6
	pmovsxbd (%rcx),%xmm4
	pmovsxwq %xmm4,%xmm6
	pmovsxwq (%rcx),%xmm4
	pmovzxbd %xmm4,%xmm6
	pmovzxbd (%rcx),%xmm4
	pmovzxwq %xmm4,%xmm6
	pmovzxwq (%rcx),%xmm4
	ucomiss %xmm4,%xmm6
	ucomiss (%rcx),%xmm4

# Tests for op mem32, xmm
	movss (%rcx),%xmm4

# Tests for op xmm, mem32
	movss %xmm4,(%rcx)

# Tests for op xmm, regl/mem32
# Tests for op regl/mem32, xmm
	movd %xmm4,%ecx
	movd %xmm4,(%rcx)
	movd %ecx,%xmm4
	movd (%rcx),%xmm4

# Tests for op xmm/mem32, regl
	cvtss2si %xmm4,%ecx
	cvtss2si (%rcx),%ecx
	cvttss2si %xmm4,%ecx
	cvttss2si (%rcx),%ecx

# Tests for op xmm/mem32, regq
	cvtss2si %xmm4,%rcx
	cvtss2si (%rcx),%rcx
	cvttss2si %xmm4,%rcx
	cvttss2si (%rcx),%rcx

# Tests for op xmm, regq
	movmskpd %xmm4,%rcx
	movmskps %xmm4,%rcx
	pmovmskb %xmm4,%rcx

# Tests for op imm8, xmm, regq/mem32
	extractps $100,%xmm4,%rcx
	extractps $100,%xmm4,(%rcx)
# Tests for op imm8, xmm, regl/mem32
	pextrd $100,%xmm4,%ecx
	pextrd $100,%xmm4,(%rcx)
	extractps $100,%xmm4,%ecx
	extractps $100,%xmm4,(%rcx)

# Tests for op regl/mem32, xmm[, xmm]
	cvtsi2sd %ecx,%xmm4
	cvtsi2sd (%rcx),%xmm4
	cvtsi2ss %ecx,%xmm4
	cvtsi2ss (%rcx),%xmm4

# Tests for op imm8, xmm/mem32, xmm[, xmm]
	cmpss $100,%xmm4,%xmm6
	cmpss $100,(%rcx),%xmm6
	insertps $100,%xmm4,%xmm6
	insertps $100,(%rcx),%xmm6
	roundss $100,%xmm4,%xmm6
	roundss $100,(%rcx),%xmm6

# Tests for op xmm/m16, xmm
	pmovsxbq %xmm4,%xmm6
	pmovsxbq (%rcx),%xmm4
	pmovzxbq %xmm4,%xmm6
	pmovzxbq (%rcx),%xmm4

# Tests for op imm8, xmm, regl/mem16
	pextrw $100,%xmm4,%ecx
	pextrw $100,%xmm4,(%rcx)

# Tests for op imm8, xmm, regq/mem16
	pextrw $100,%xmm4,%rcx
	pextrw $100,%xmm4,(%rcx)

# Tests for op imm8, regl/mem16, xmm[, xmm]
	pinsrw $100,%ecx,%xmm4
	pinsrw $100,(%rcx),%xmm4


	pinsrw $100,%rcx,%xmm4
	pinsrw $100,(%rcx),%xmm4

# Tests for op imm8, xmm, regl/mem8
	pextrb $100,%xmm4,%ecx
	pextrb $100,%xmm4,(%rcx)

# Tests for op imm8, regl/mem8, xmm[, xmm]
	pinsrb $100,%ecx,%xmm4
	pinsrb $100,(%rcx),%xmm4

# Tests for op imm8, xmm, regq
	pextrw $100,%xmm4,%rcx
# Tests for op imm8, xmm, regq/mem8
	pextrb $100,%xmm4,%rcx
	pextrb $100,%xmm4,(%rcx)

# Tests for op imm8, regl/mem8, xmm[, xmm]
	pinsrb $100,%ecx,%xmm4
	pinsrb $100,(%rcx),%xmm4

# Tests for op xmm, xmm
	maskmovdqu %xmm4,%xmm6
	movq %xmm4,%xmm6

# Tests for op xmm, regl
	movmskpd %xmm4,%ecx
	movmskps %xmm4,%ecx
	pmovmskb %xmm4,%ecx
# Tests for op xmm, xmm[, xmm]
	movhlps %xmm4,%xmm6
	movlhps %xmm4,%xmm6
	movsd %xmm4,%xmm6
	movss %xmm4,%xmm6

# Tests for op imm8, xmm[, xmm]
	pslld $100,%xmm4
	pslldq $100,%xmm4
	psllq $100,%xmm4
	psllw $100,%xmm4
	psrad $100,%xmm4
	psraw $100,%xmm4
	psrld $100,%xmm4
	psrldq $100,%xmm4
	psrlq $100,%xmm4
	psrlw $100,%xmm4

# Tests for op imm8, xmm, regl
	pextrw $100,%xmm4,%ecx

