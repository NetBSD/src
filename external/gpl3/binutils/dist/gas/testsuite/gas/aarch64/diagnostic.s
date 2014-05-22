// diagnostic.s Test file for diagnostic quality.

.text
	fmul,	s0, s1, s2
	fmul	,	s0, s1, s2
	fmul	, s0, s1, s2
	b.random	label1
	fmull   s0
	aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	sys	1,c1,c3,3,
	ext	v0.8b, v1.8b, v2.8b, 8
	ext	v0.16b, v1.16b, v2.16b, 20
	svc	-1
	svc	65536
	ccmp	w0, 32, 10, le
	ccmp	x0, -1, 10, le
	tlbi	alle3is, x0
	tlbi	vaale1is
	tlbi	vaale1is x0
	msr	spsel, 3
	fcvtzu	x15, d31, #66
	scvtf	s0, w0, 33
	scvtf	s0, w0, 0
	smlal	v0.4s, v31.4h, v16.h[1]
	smlal	v0.4s, v31.4h, v15.h[8]
	add	sp, x0, x7, lsr #2
	add	x0, x0, x7, uxtx #5
	add	x0, xzr, x7, ror #5
	add	w0, wzr, w7, asr #32
	st2	{v0.4s, v1.4s}, [sp], #24
	ldr	q0, [x0, w0, uxtw #5]
	st2	{v0.4s, v1.4s, v2.4s, v3.4s}, [sp], #64
	adds	x1, sp, 2134, lsl #4
	movz	w0, 2134, lsl #8
	movz	w0, 2134, lsl #32
	movz	x0, 2134, lsl #47
	sbfiz	w0, w7, 15, 18
	sbfiz	w0, w7, 15, 0
	shll	v1.4s, v2.4h, #15
	shll	v1.4s, v2.4h, #32
	shl	v1.2s, v2.2s, 32
	sqshrn2	v2.16b, v3.8h, #17
	movi	v1.4h, 256
	movi	v1.4h, -1
	movi	v1.4h, 255, msl #8
	movi	d0, 256
	movi	v1.4h, 255, lsl #7
	movi	v1.4h, 255, lsl #16
	movi	v2.2s, 255, msl #0
	movi	v2.2s, 255, msl #15
	fmov	v1.2s, 1.01
	fmov	v1.2d, 1.01
	fmov	s3, 1.01
	fmov	d3, 1.01
	fcmp	d0, #1.0
	fcmp	d0, x0
	cmgt	v0.4s, v2.4s, #1
	fmov	d3, 1.00, lsl #3
	st2	{v0.4s, v1.4s}, [sp], sp
	st2	{v0.4s, v1.4s}, [sp], zr
	ldr	q0, [x0, w0, lsr #4]
	adds	x1, sp, 2134, uxtw #12
	movz	x0, 2134, lsl #64
	adds	sp, sp, 2134, lsl #12
	ldxrb	w2, [x0, #1]
	ldrb	w0, x1, x2, sxtx
	prfm	PLDL3KEEP, [x9, x15, sxtx #2]
	sysl	x7, #1, C16, C30, #1
	sysl	x7, #1, C15, C77, #1
	add	x0, xzr, x7, uxtx #5
	mov	x0, ##5
	bad expression
	mockup-op
	orr	x0. x0, #0xff, lsl #1
	movk	x1, #:abs_g1_s:s12
	movz	x1, #:abs_g1_s:s12, lsl #16
	prfm	pldl3strm, [sp, w0, sxtw #3]!
	prfm	0x2f, LABEL1
	dmb	#16
	tbz	w0, #40, 0x17c
	st2	{v4.2d, v5.2d, v6.2d}, [x3]
	ld2	{v1.4h, v0.4h}, [x1]
	isb	osh
	st2	{v4.2d, v5.2d, v6.2d}, \[x3\]
	ldnp	w7, w15, [x3, #3]
	stnp	x7, x15, [x3, #32]!
	ldnp	w7, w15, [x3, #256]
	movi	v1.2d, 4294967295, lsl #0
	movi	v1.8b, 97, lsl #8
	msr	dummy, x1
	fmov	s0, 0x42000000
