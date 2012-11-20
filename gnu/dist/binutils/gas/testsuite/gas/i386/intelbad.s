	.intel_syntax noprefix
	.text
start:
	add	eax, byte ptr [eax]
	add	eax, qword ptr [eax]
	add	[eax], 1
	add	qword ptr [eax], 1
	addpd	xmm0, dword ptr [eax]
	addpd	xmm0, qword ptr [eax]
	addpd	xmm0, tbyte ptr [eax]
	addps	xmm0, dword ptr [eax]
	addps	xmm0, qword ptr [eax]
	addps	xmm0, tbyte ptr [eax]
	addsd	xmm0, dword ptr [eax]
	addsd	xmm0, tbyte ptr [eax]
	addsd	xmm0, xmmword ptr [eax]
	addss	xmm0, qword ptr [eax]
	addss	xmm0, tbyte ptr [eax]
	addss	xmm0, xmmword ptr [eax]
	call	byte ptr [eax]
	call	qword ptr [eax]
	call	tbyte ptr [eax]
	call	xword ptr [eax]
	cmps	[esi], es:[edi]
	cmps	dword ptr [esi], word ptr es:[edi]
	cmpxchg8b dword ptr [eax]
	fadd	[eax]
	fadd	word ptr [eax]
	fadd	tbyte ptr [eax]
	fbld	byte ptr [eax]
	fbld	word ptr [eax]
	fbstp	dword ptr [eax]
	fbstp	qword ptr [eax]
	fiadd	[eax]
	fiadd	byte ptr [eax]
	fild	[eax]
	fild	byte ptr [eax]
	fild	tbyte ptr [eax]
	fist	[eax]
	fist	byte ptr [eax]
	fist	qword ptr [eax]
	fistp	[eax]
	fistp	byte ptr [eax]
	fisttp	[eax]
	fisttp	byte ptr [eax]
	fld	[eax]
	fld	word ptr [eax]
	fldcw	dword ptr [eax]
	fst	[eax]
	fst	word ptr [eax]
	fst	tbyte ptr [eax]
	fstp	[eax]
	fstp	word ptr [eax]
	ins	es:[edi], dx
	lds	ax, word ptr [eax]
	lds	eax, dword ptr [eax]
	lods	[esi]
	movs	es:[edi], [esi]
	movs	dword ptr es:[edi], word ptr [esi]
	movsx	eax, [eax]
	movsx	eax, dword ptr [eax]
	outs	dx, [esi]
	paddb	mm0, dword ptr [eax]
	paddb	mm0, xmmword ptr [eax]
	paddb	xmm0, dword ptr [eax]
	paddb	xmm0, qword ptr [eax]
	pinsrw	mm0, byte ptr [eax], 3
	pinsrw	mm0, dword ptr [eax], 3
	pinsrw	mm0, qword ptr [eax], 3
	pinsrw	xmm0, dword ptr [eax], 7
	pinsrw	xmm0, qword ptr [eax], 7
	pinsrw	xmm0, xmmword ptr [eax], 7
	push	byte ptr [eax]
	push	qword ptr [eax]
	scas	es:[edi]
#XXX?	shl	eax
	stos	es:[edi]
	xlat	word ptr [ebx]
#XXX?	xlatb	[ebx]

	# expressions
#XXX?	push	~ 1
#XXX?	push	1 % 1
#XXX?	push	1 << 1
#XXX?	push	1 >> 1
#XXX?	push	1 & 1
#XXX?	push	1 ^ 1
#XXX?	push	1 | 1
	push	1 1
	push	1 +
	push	1 * * 1
