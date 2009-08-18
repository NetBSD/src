# Check SSE to AVX instructions

	.allow_index_reg
	.text
_start:
# Tests for op mem64
	ldmxcsr (%ecx)
	stmxcsr (%ecx)

# Tests for op xmm/mem128, xmm
	cvtdq2ps %xmm4,%xmm6
	cvtdq2ps (%ecx),%xmm4
	cvtpd2dq %xmm4,%xmm6
	cvtpd2dq (%ecx),%xmm4
	cvtpd2ps %xmm4,%xmm6
	cvtpd2ps (%ecx),%xmm4
	cvtps2dq %xmm4,%xmm6
	cvtps2dq (%ecx),%xmm4
	cvttpd2dq %xmm4,%xmm6
	cvttpd2dq (%ecx),%xmm4
	cvttps2dq %xmm4,%xmm6
	cvttps2dq (%ecx),%xmm4
	movapd %xmm4,%xmm6
	movapd (%ecx),%xmm4
	movaps %xmm4,%xmm6
	movaps (%ecx),%xmm4
	movdqa %xmm4,%xmm6
	movdqa (%ecx),%xmm4
	movdqu %xmm4,%xmm6
	movdqu (%ecx),%xmm4
	movshdup %xmm4,%xmm6
	movshdup (%ecx),%xmm4
	movsldup %xmm4,%xmm6
	movsldup (%ecx),%xmm4
	movupd %xmm4,%xmm6
	movupd (%ecx),%xmm4
	movups %xmm4,%xmm6
	movups (%ecx),%xmm4
	pabsb %xmm4,%xmm6
	pabsb (%ecx),%xmm4
	pabsw %xmm4,%xmm6
	pabsw (%ecx),%xmm4
	pabsd %xmm4,%xmm6
	pabsd (%ecx),%xmm4
	phminposuw %xmm4,%xmm6
	phminposuw (%ecx),%xmm4
	ptest %xmm4,%xmm6
	ptest (%ecx),%xmm4
	rcpps %xmm4,%xmm6
	rcpps (%ecx),%xmm4
	rsqrtps %xmm4,%xmm6
	rsqrtps (%ecx),%xmm4
	sqrtpd %xmm4,%xmm6
	sqrtpd (%ecx),%xmm4
	sqrtps %xmm4,%xmm6
	sqrtps (%ecx),%xmm4
	aesimc %xmm4,%xmm6
	aesimc (%ecx),%xmm4

# Tests for op xmm, xmm/mem128
	movapd %xmm4,%xmm6
	movapd %xmm4,(%ecx)
	movaps %xmm4,%xmm6
	movaps %xmm4,(%ecx)
	movdqa %xmm4,%xmm6
	movdqa %xmm4,(%ecx)
	movdqu %xmm4,%xmm6
	movdqu %xmm4,(%ecx)
	movupd %xmm4,%xmm6
	movupd %xmm4,(%ecx)
	movups %xmm4,%xmm6
	movups %xmm4,(%ecx)

# Tests for op mem128, xmm
	lddqu (%ecx),%xmm4
	movntdqa (%ecx),%xmm4

# Tests for op xmm, mem128
	movntdq %xmm4,(%ecx)
	movntpd %xmm4,(%ecx)
	movntps %xmm4,(%ecx)

# Tests for op xmm/mem128, xmm[, xmm]
	addpd %xmm4,%xmm6
	addpd (%ecx),%xmm6
	addps %xmm4,%xmm6
	addps (%ecx),%xmm6
	addsubpd %xmm4,%xmm6
	addsubpd (%ecx),%xmm6
	addsubps %xmm4,%xmm6
	addsubps (%ecx),%xmm6
	andnpd %xmm4,%xmm6
	andnpd (%ecx),%xmm6
	andnps %xmm4,%xmm6
	andnps (%ecx),%xmm6
	andpd %xmm4,%xmm6
	andpd (%ecx),%xmm6
	andps %xmm4,%xmm6
	andps (%ecx),%xmm6
	divpd %xmm4,%xmm6
	divpd (%ecx),%xmm6
	divps %xmm4,%xmm6
	divps (%ecx),%xmm6
	haddpd %xmm4,%xmm6
	haddpd (%ecx),%xmm6
	haddps %xmm4,%xmm6
	haddps (%ecx),%xmm6
	hsubpd %xmm4,%xmm6
	hsubpd (%ecx),%xmm6
	hsubps %xmm4,%xmm6
	hsubps (%ecx),%xmm6
	maxpd %xmm4,%xmm6
	maxpd (%ecx),%xmm6
	maxps %xmm4,%xmm6
	maxps (%ecx),%xmm6
	minpd %xmm4,%xmm6
	minpd (%ecx),%xmm6
	minps %xmm4,%xmm6
	minps (%ecx),%xmm6
	mulpd %xmm4,%xmm6
	mulpd (%ecx),%xmm6
	mulps %xmm4,%xmm6
	mulps (%ecx),%xmm6
	orpd %xmm4,%xmm6
	orpd (%ecx),%xmm6
	orps %xmm4,%xmm6
	orps (%ecx),%xmm6
	packsswb %xmm4,%xmm6
	packsswb (%ecx),%xmm6
	packssdw %xmm4,%xmm6
	packssdw (%ecx),%xmm6
	packuswb %xmm4,%xmm6
	packuswb (%ecx),%xmm6
	packusdw %xmm4,%xmm6
	packusdw (%ecx),%xmm6
	paddb %xmm4,%xmm6
	paddb (%ecx),%xmm6
	paddw %xmm4,%xmm6
	paddw (%ecx),%xmm6
	paddd %xmm4,%xmm6
	paddd (%ecx),%xmm6
	paddq %xmm4,%xmm6
	paddq (%ecx),%xmm6
	paddsb %xmm4,%xmm6
	paddsb (%ecx),%xmm6
	paddsw %xmm4,%xmm6
	paddsw (%ecx),%xmm6
	paddusb %xmm4,%xmm6
	paddusb (%ecx),%xmm6
	paddusw %xmm4,%xmm6
	paddusw (%ecx),%xmm6
	pand %xmm4,%xmm6
	pand (%ecx),%xmm6
	pandn %xmm4,%xmm6
	pandn (%ecx),%xmm6
	pavgb %xmm4,%xmm6
	pavgb (%ecx),%xmm6
	pavgw %xmm4,%xmm6
	pavgw (%ecx),%xmm6
	pclmullqlqdq %xmm4,%xmm6
	pclmullqlqdq (%ecx),%xmm6
	pclmulhqlqdq %xmm4,%xmm6
	pclmulhqlqdq (%ecx),%xmm6
	pclmullqhqdq %xmm4,%xmm6
	pclmullqhqdq (%ecx),%xmm6
	pclmulhqhqdq %xmm4,%xmm6
	pclmulhqhqdq (%ecx),%xmm6
	pcmpeqb %xmm4,%xmm6
	pcmpeqb (%ecx),%xmm6
	pcmpeqw %xmm4,%xmm6
	pcmpeqw (%ecx),%xmm6
	pcmpeqd %xmm4,%xmm6
	pcmpeqd (%ecx),%xmm6
	pcmpeqq %xmm4,%xmm6
	pcmpeqq (%ecx),%xmm6
	pcmpgtb %xmm4,%xmm6
	pcmpgtb (%ecx),%xmm6
	pcmpgtw %xmm4,%xmm6
	pcmpgtw (%ecx),%xmm6
	pcmpgtd %xmm4,%xmm6
	pcmpgtd (%ecx),%xmm6
	pcmpgtq %xmm4,%xmm6
	pcmpgtq (%ecx),%xmm6
	phaddw %xmm4,%xmm6
	phaddw (%ecx),%xmm6
	phaddd %xmm4,%xmm6
	phaddd (%ecx),%xmm6
	phaddsw %xmm4,%xmm6
	phaddsw (%ecx),%xmm6
	phsubw %xmm4,%xmm6
	phsubw (%ecx),%xmm6
	phsubd %xmm4,%xmm6
	phsubd (%ecx),%xmm6
	phsubsw %xmm4,%xmm6
	phsubsw (%ecx),%xmm6
	pmaddwd %xmm4,%xmm6
	pmaddwd (%ecx),%xmm6
	pmaddubsw %xmm4,%xmm6
	pmaddubsw (%ecx),%xmm6
	pmaxsb %xmm4,%xmm6
	pmaxsb (%ecx),%xmm6
	pmaxsw %xmm4,%xmm6
	pmaxsw (%ecx),%xmm6
	pmaxsd %xmm4,%xmm6
	pmaxsd (%ecx),%xmm6
	pmaxub %xmm4,%xmm6
	pmaxub (%ecx),%xmm6
	pmaxuw %xmm4,%xmm6
	pmaxuw (%ecx),%xmm6
	pmaxud %xmm4,%xmm6
	pmaxud (%ecx),%xmm6
	pminsb %xmm4,%xmm6
	pminsb (%ecx),%xmm6
	pminsw %xmm4,%xmm6
	pminsw (%ecx),%xmm6
	pminsd %xmm4,%xmm6
	pminsd (%ecx),%xmm6
	pminub %xmm4,%xmm6
	pminub (%ecx),%xmm6
	pminuw %xmm4,%xmm6
	pminuw (%ecx),%xmm6
	pminud %xmm4,%xmm6
	pminud (%ecx),%xmm6
	pmulhuw %xmm4,%xmm6
	pmulhuw (%ecx),%xmm6
	pmulhrsw %xmm4,%xmm6
	pmulhrsw (%ecx),%xmm6
	pmulhw %xmm4,%xmm6
	pmulhw (%ecx),%xmm6
	pmullw %xmm4,%xmm6
	pmullw (%ecx),%xmm6
	pmulld %xmm4,%xmm6
	pmulld (%ecx),%xmm6
	pmuludq %xmm4,%xmm6
	pmuludq (%ecx),%xmm6
	pmuldq %xmm4,%xmm6
	pmuldq (%ecx),%xmm6
	por %xmm4,%xmm6
	por (%ecx),%xmm6
	psadbw %xmm4,%xmm6
	psadbw (%ecx),%xmm6
	pshufb %xmm4,%xmm6
	pshufb (%ecx),%xmm6
	psignb %xmm4,%xmm6
	psignb (%ecx),%xmm6
	psignw %xmm4,%xmm6
	psignw (%ecx),%xmm6
	psignd %xmm4,%xmm6
	psignd (%ecx),%xmm6
	psllw %xmm4,%xmm6
	psllw (%ecx),%xmm6
	pslld %xmm4,%xmm6
	pslld (%ecx),%xmm6
	psllq %xmm4,%xmm6
	psllq (%ecx),%xmm6
	psraw %xmm4,%xmm6
	psraw (%ecx),%xmm6
	psrad %xmm4,%xmm6
	psrad (%ecx),%xmm6
	psrlw %xmm4,%xmm6
	psrlw (%ecx),%xmm6
	psrld %xmm4,%xmm6
	psrld (%ecx),%xmm6
	psrlq %xmm4,%xmm6
	psrlq (%ecx),%xmm6
	psubb %xmm4,%xmm6
	psubb (%ecx),%xmm6
	psubw %xmm4,%xmm6
	psubw (%ecx),%xmm6
	psubd %xmm4,%xmm6
	psubd (%ecx),%xmm6
	psubq %xmm4,%xmm6
	psubq (%ecx),%xmm6
	psubsb %xmm4,%xmm6
	psubsb (%ecx),%xmm6
	psubsw %xmm4,%xmm6
	psubsw (%ecx),%xmm6
	psubusb %xmm4,%xmm6
	psubusb (%ecx),%xmm6
	psubusw %xmm4,%xmm6
	psubusw (%ecx),%xmm6
	punpckhbw %xmm4,%xmm6
	punpckhbw (%ecx),%xmm6
	punpckhwd %xmm4,%xmm6
	punpckhwd (%ecx),%xmm6
	punpckhdq %xmm4,%xmm6
	punpckhdq (%ecx),%xmm6
	punpckhqdq %xmm4,%xmm6
	punpckhqdq (%ecx),%xmm6
	punpcklbw %xmm4,%xmm6
	punpcklbw (%ecx),%xmm6
	punpcklwd %xmm4,%xmm6
	punpcklwd (%ecx),%xmm6
	punpckldq %xmm4,%xmm6
	punpckldq (%ecx),%xmm6
	punpcklqdq %xmm4,%xmm6
	punpcklqdq (%ecx),%xmm6
	pxor %xmm4,%xmm6
	pxor (%ecx),%xmm6
	subpd %xmm4,%xmm6
	subpd (%ecx),%xmm6
	subps %xmm4,%xmm6
	subps (%ecx),%xmm6
	unpckhpd %xmm4,%xmm6
	unpckhpd (%ecx),%xmm6
	unpckhps %xmm4,%xmm6
	unpckhps (%ecx),%xmm6
	unpcklpd %xmm4,%xmm6
	unpcklpd (%ecx),%xmm6
	unpcklps %xmm4,%xmm6
	unpcklps (%ecx),%xmm6
	xorpd %xmm4,%xmm6
	xorpd (%ecx),%xmm6
	xorps %xmm4,%xmm6
	xorps (%ecx),%xmm6
	aesenc %xmm4,%xmm6
	aesenc (%ecx),%xmm6
	aesenclast %xmm4,%xmm6
	aesenclast (%ecx),%xmm6
	aesdec %xmm4,%xmm6
	aesdec (%ecx),%xmm6
	aesdeclast %xmm4,%xmm6
	aesdeclast (%ecx),%xmm6
	cmpeqpd %xmm4,%xmm6
	cmpeqpd (%ecx),%xmm6
	cmpeqps %xmm4,%xmm6
	cmpeqps (%ecx),%xmm6
	cmpltpd %xmm4,%xmm6
	cmpltpd (%ecx),%xmm6
	cmpltps %xmm4,%xmm6
	cmpltps (%ecx),%xmm6
	cmplepd %xmm4,%xmm6
	cmplepd (%ecx),%xmm6
	cmpleps %xmm4,%xmm6
	cmpleps (%ecx),%xmm6
	cmpunordpd %xmm4,%xmm6
	cmpunordpd (%ecx),%xmm6
	cmpunordps %xmm4,%xmm6
	cmpunordps (%ecx),%xmm6
	cmpneqpd %xmm4,%xmm6
	cmpneqpd (%ecx),%xmm6
	cmpneqps %xmm4,%xmm6
	cmpneqps (%ecx),%xmm6
	cmpnltpd %xmm4,%xmm6
	cmpnltpd (%ecx),%xmm6
	cmpnltps %xmm4,%xmm6
	cmpnltps (%ecx),%xmm6
	cmpnlepd %xmm4,%xmm6
	cmpnlepd (%ecx),%xmm6
	cmpnleps %xmm4,%xmm6
	cmpnleps (%ecx),%xmm6
	cmpordpd %xmm4,%xmm6
	cmpordpd (%ecx),%xmm6
	cmpordps %xmm4,%xmm6
	cmpordps (%ecx),%xmm6

# Tests for op imm8, xmm/mem128, xmm
	aeskeygenassist $100,%xmm4,%xmm6
	aeskeygenassist $100,(%ecx),%xmm6
	pcmpestri $100,%xmm4,%xmm6
	pcmpestri $100,(%ecx),%xmm6
	pcmpestrm $100,%xmm4,%xmm6
	pcmpestrm $100,(%ecx),%xmm6
	pcmpistri $100,%xmm4,%xmm6
	pcmpistri $100,(%ecx),%xmm6
	pcmpistrm $100,%xmm4,%xmm6
	pcmpistrm $100,(%ecx),%xmm6
	pshufd $100,%xmm4,%xmm6
	pshufd $100,(%ecx),%xmm6
	pshufhw $100,%xmm4,%xmm6
	pshufhw $100,(%ecx),%xmm6
	pshuflw $100,%xmm4,%xmm6
	pshuflw $100,(%ecx),%xmm6
	roundpd $100,%xmm4,%xmm6
	roundpd $100,(%ecx),%xmm6
	roundps $100,%xmm4,%xmm6
	roundps $100,(%ecx),%xmm6

# Tests for op imm8, xmm/mem128, xmm[, xmm]
	blendpd $100,%xmm4,%xmm6
	blendpd $100,(%ecx),%xmm6
	blendps $100,%xmm4,%xmm6
	blendps $100,(%ecx),%xmm6
	cmppd $100,%xmm4,%xmm6
	cmppd $100,(%ecx),%xmm6
	cmpps $100,%xmm4,%xmm6
	cmpps $100,(%ecx),%xmm6
	dppd $100,%xmm4,%xmm6
	dppd $100,(%ecx),%xmm6
	dpps $100,%xmm4,%xmm6
	dpps $100,(%ecx),%xmm6
	mpsadbw $100,%xmm4,%xmm6
	mpsadbw $100,(%ecx),%xmm6
	palignr $100,%xmm4,%xmm6
	palignr $100,(%ecx),%xmm6
	pblendw $100,%xmm4,%xmm6
	pblendw $100,(%ecx),%xmm6
	shufpd $100,%xmm4,%xmm6
	shufpd $100,(%ecx),%xmm6
	shufps $100,%xmm4,%xmm6
	shufps $100,(%ecx),%xmm6

# Tests for op xmm0, xmm/mem128, xmm[, xmm]
	blendvpd %xmm0,%xmm4,%xmm6
	blendvpd %xmm0,(%ecx),%xmm6
	blendvpd %xmm4,%xmm6
	blendvpd (%ecx),%xmm6
	blendvps %xmm0,%xmm4,%xmm6
	blendvps %xmm0,(%ecx),%xmm6
	blendvps %xmm4,%xmm6
	blendvps (%ecx),%xmm6
	pblendvb %xmm0,%xmm4,%xmm6
	pblendvb %xmm0,(%ecx),%xmm6
	pblendvb %xmm4,%xmm6
	pblendvb (%ecx),%xmm6

# Tests for op xmm/mem64, xmm
	comisd %xmm4,%xmm6
	comisd (%ecx),%xmm4
	cvtdq2pd %xmm4,%xmm6
	cvtdq2pd (%ecx),%xmm4
	cvtps2pd %xmm4,%xmm6
	cvtps2pd (%ecx),%xmm4
	movddup %xmm4,%xmm6
	movddup (%ecx),%xmm4
	pmovsxbw %xmm4,%xmm6
	pmovsxbw (%ecx),%xmm4
	pmovsxwd %xmm4,%xmm6
	pmovsxwd (%ecx),%xmm4
	pmovsxdq %xmm4,%xmm6
	pmovsxdq (%ecx),%xmm4
	pmovzxbw %xmm4,%xmm6
	pmovzxbw (%ecx),%xmm4
	pmovzxwd %xmm4,%xmm6
	pmovzxwd (%ecx),%xmm4
	pmovzxdq %xmm4,%xmm6
	pmovzxdq (%ecx),%xmm4
	ucomisd %xmm4,%xmm6
	ucomisd (%ecx),%xmm4

# Tests for op mem64, xmm
	movsd (%ecx),%xmm4

# Tests for op xmm, mem64
	movlpd %xmm4,(%ecx)
	movlps %xmm4,(%ecx)
	movhpd %xmm4,(%ecx)
	movhps %xmm4,(%ecx)
	movsd %xmm4,(%ecx)

# Tests for op xmm, regq/mem64
# Tests for op regq/mem64, xmm
	movq %xmm4,(%ecx)
	movq (%ecx),%xmm4

# Tests for op xmm/mem64, regl
	cvtsd2si %xmm4,%ecx
	cvtsd2si (%ecx),%ecx
	cvttsd2si %xmm4,%ecx
	cvttsd2si (%ecx),%ecx

# Tests for op mem64, xmm[, xmm]
	movlpd (%ecx),%xmm4
	movlps (%ecx),%xmm4
	movhpd (%ecx),%xmm4
	movhps (%ecx),%xmm4

# Tests for op imm8, xmm/mem64, xmm[, xmm]
	cmpsd $100,%xmm4,%xmm6
	cmpsd $100,(%ecx),%xmm6
	roundsd $100,%xmm4,%xmm6
	roundsd $100,(%ecx),%xmm6

# Tests for op xmm/mem64, xmm[, xmm]
	addsd %xmm4,%xmm6
	addsd (%ecx),%xmm6
	cvtsd2ss %xmm4,%xmm6
	cvtsd2ss (%ecx),%xmm6
	divsd %xmm4,%xmm6
	divsd (%ecx),%xmm6
	maxsd %xmm4,%xmm6
	maxsd (%ecx),%xmm6
	minsd %xmm4,%xmm6
	minsd (%ecx),%xmm6
	mulsd %xmm4,%xmm6
	mulsd (%ecx),%xmm6
	sqrtsd %xmm4,%xmm6
	sqrtsd (%ecx),%xmm6
	subsd %xmm4,%xmm6
	subsd (%ecx),%xmm6
	cmpeqsd %xmm4,%xmm6
	cmpeqsd (%ecx),%xmm6
	cmpltsd %xmm4,%xmm6
	cmpltsd (%ecx),%xmm6
	cmplesd %xmm4,%xmm6
	cmplesd (%ecx),%xmm6
	cmpunordsd %xmm4,%xmm6
	cmpunordsd (%ecx),%xmm6
	cmpneqsd %xmm4,%xmm6
	cmpneqsd (%ecx),%xmm6
	cmpnltsd %xmm4,%xmm6
	cmpnltsd (%ecx),%xmm6
	cmpnlesd %xmm4,%xmm6
	cmpnlesd (%ecx),%xmm6
	cmpordsd %xmm4,%xmm6
	cmpordsd (%ecx),%xmm6

# Tests for op xmm/mem32, xmm[, xmm]
	addss %xmm4,%xmm6
	addss (%ecx),%xmm6
	cvtss2sd %xmm4,%xmm6
	cvtss2sd (%ecx),%xmm6
	divss %xmm4,%xmm6
	divss (%ecx),%xmm6
	maxss %xmm4,%xmm6
	maxss (%ecx),%xmm6
	minss %xmm4,%xmm6
	minss (%ecx),%xmm6
	mulss %xmm4,%xmm6
	mulss (%ecx),%xmm6
	rcpss %xmm4,%xmm6
	rcpss (%ecx),%xmm6
	rsqrtss %xmm4,%xmm6
	rsqrtss (%ecx),%xmm6
	sqrtss %xmm4,%xmm6
	sqrtss (%ecx),%xmm6
	subss %xmm4,%xmm6
	subss (%ecx),%xmm6
	cmpeqss %xmm4,%xmm6
	cmpeqss (%ecx),%xmm6
	cmpltss %xmm4,%xmm6
	cmpltss (%ecx),%xmm6
	cmpless %xmm4,%xmm6
	cmpless (%ecx),%xmm6
	cmpunordss %xmm4,%xmm6
	cmpunordss (%ecx),%xmm6
	cmpneqss %xmm4,%xmm6
	cmpneqss (%ecx),%xmm6
	cmpnltss %xmm4,%xmm6
	cmpnltss (%ecx),%xmm6
	cmpnless %xmm4,%xmm6
	cmpnless (%ecx),%xmm6
	cmpordss %xmm4,%xmm6
	cmpordss (%ecx),%xmm6

# Tests for op xmm/mem32, xmm
	comiss %xmm4,%xmm6
	comiss (%ecx),%xmm4
	pmovsxbd %xmm4,%xmm6
	pmovsxbd (%ecx),%xmm4
	pmovsxwq %xmm4,%xmm6
	pmovsxwq (%ecx),%xmm4
	pmovzxbd %xmm4,%xmm6
	pmovzxbd (%ecx),%xmm4
	pmovzxwq %xmm4,%xmm6
	pmovzxwq (%ecx),%xmm4
	ucomiss %xmm4,%xmm6
	ucomiss (%ecx),%xmm4

# Tests for op mem32, xmm
	movss (%ecx),%xmm4

# Tests for op xmm, mem32
	movss %xmm4,(%ecx)

# Tests for op xmm, regl/mem32
# Tests for op regl/mem32, xmm
	movd %xmm4,%ecx
	movd %xmm4,(%ecx)
	movd %ecx,%xmm4
	movd (%ecx),%xmm4

# Tests for op xmm/mem32, regl
	cvtss2si %xmm4,%ecx
	cvtss2si (%ecx),%ecx
	cvttss2si %xmm4,%ecx
	cvttss2si (%ecx),%ecx

# Tests for op imm8, xmm, regq/mem32
	extractps $100,%xmm4,(%ecx)
# Tests for op imm8, xmm, regl/mem32
	pextrd $100,%xmm4,%ecx
	pextrd $100,%xmm4,(%ecx)
	extractps $100,%xmm4,%ecx
	extractps $100,%xmm4,(%ecx)

# Tests for op regl/mem32, xmm[, xmm]
	cvtsi2sd %ecx,%xmm4
	cvtsi2sd (%ecx),%xmm4
	cvtsi2ss %ecx,%xmm4
	cvtsi2ss (%ecx),%xmm4

# Tests for op imm8, xmm/mem32, xmm[, xmm]
	cmpss $100,%xmm4,%xmm6
	cmpss $100,(%ecx),%xmm6
	insertps $100,%xmm4,%xmm6
	insertps $100,(%ecx),%xmm6
	roundss $100,%xmm4,%xmm6
	roundss $100,(%ecx),%xmm6

# Tests for op xmm/m16, xmm
	pmovsxbq %xmm4,%xmm6
	pmovsxbq (%ecx),%xmm4
	pmovzxbq %xmm4,%xmm6
	pmovzxbq (%ecx),%xmm4

# Tests for op imm8, xmm, regl/mem16
	pextrw $100,%xmm4,%ecx
	pextrw $100,%xmm4,(%ecx)

# Tests for op imm8, xmm, regq/mem16
	pextrw $100,%xmm4,(%ecx)

# Tests for op imm8, regl/mem16, xmm[, xmm]
	pinsrw $100,%ecx,%xmm4
	pinsrw $100,(%ecx),%xmm4


# Tests for op imm8, xmm, regl/mem8
	pextrb $100,%xmm4,%ecx
	pextrb $100,%xmm4,(%ecx)

# Tests for op imm8, regl/mem8, xmm[, xmm]
	pinsrb $100,%ecx,%xmm4
	pinsrb $100,(%ecx),%xmm4

# Tests for op imm8, xmm, regq/mem8
	pextrb $100,%xmm4,(%ecx)

# Tests for op imm8, regl/mem8, xmm[, xmm]
	pinsrb $100,%ecx,%xmm4
	pinsrb $100,(%ecx),%xmm4

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

