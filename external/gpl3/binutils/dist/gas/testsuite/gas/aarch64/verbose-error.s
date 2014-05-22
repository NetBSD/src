// verbose-error.s Test file for -mverbose-error

.text
	strb	w7, [x30, x0, lsl]
	ubfm	w0, x1, 8, 31
	bfm	w0, w1, 8, 43
	strb	w7, [x30, x0, lsl #1]
	st2	{v4.2d,v5.2d},[x3,#3]
	fmov	v1.D[0],x0
	ld1r	{v1.4s, v2.4s, v3.4s}, [x3], x4
	svc
	add	v0.4s, v1.4s, v2.2s
	urecpe	v0.1d,v7.1d
	adds	w0, wsp, x0, uxtx #1
	fmov	d0, s0
	ldnp	h3, h7, [sp], #16
