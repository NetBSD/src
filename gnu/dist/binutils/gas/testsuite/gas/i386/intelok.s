	.intel_syntax noprefix
	.equiv dword, 4
	.text
start:

	# operand sizes

	add	al, [eax]
	add	al, byte ptr [eax]
	add	ax, [eax]
	add	ax, word ptr [eax]
	add	eax, [eax]
	add	eax, dword ptr [eax]
	add	byte ptr [eax], 1
	add	dword ptr [eax], 1
	add	word ptr [eax], 1
	addpd	xmm0, [eax]
	addpd	xmm0, xmmword ptr [eax]
	addps	xmm0, [eax]
	addps	xmm0, xmmword ptr [eax]
	addsd	xmm0, [eax]
	addsd	xmm0, qword ptr [eax]
	addss	xmm0, [eax]
	addss	xmm0, dword ptr [eax]
	call	word ptr [eax]
	call	dword ptr [eax]
	call	fword ptr [eax]
	cmps	[esi], byte ptr es:[edi]
	cmps	dword ptr [esi], es:[edi]
	cmps	word ptr [esi], word ptr es:[edi]
	cmpxchg8b qword ptr [eax]
	fadd	dword ptr [eax]
	fadd	qword ptr [eax]
	fbld	[eax]
	fbld	tbyte ptr [eax]
	fbstp	[eax]
	fbstp	tbyte ptr [eax]
	fiadd	dword ptr [eax]
	fiadd	word ptr [eax]
	fild	dword ptr [eax]
	fild	qword ptr [eax]
	fild	word ptr [eax]
	fist	dword ptr [eax]
	fist	word ptr [eax]
	fistp	dword ptr [eax]
	fistp	qword ptr [eax]
	fistp	word ptr [eax]
	fisttp	dword ptr [eax]
	fisttp	qword ptr [eax]
	fisttp	word ptr [eax]
	fld	dword ptr [eax]
	fld	qword ptr [eax]
	fld	tbyte ptr [eax]
	fldcw	[eax]
	fldcw	word ptr [eax]
	fldenv	[eax]
	fldenvd	[eax]
	fldenvw	[eax]
	fst	dword ptr [eax]
	fst	qword ptr [eax]
	fstp	dword ptr [eax]
	fstp	qword ptr [eax]
	fstp	tbyte ptr [eax]
	lds	ax, [eax]
	lds	eax, [eax]
	lds	ax, dword ptr [eax]
	lds	eax, fword ptr [eax]
	lea	eax, [eax]
	lea	eax, byte ptr [eax]
	lea	eax, dword ptr [eax]
	lea	eax, fword ptr [eax]
	lea	eax, qword ptr [eax]
	lea	eax, tbyte ptr [eax]
	lea	eax, word ptr [eax]
	lea	eax, xmmword ptr [eax]
	lgdt	[eax]
	lgdtd	[eax]
	lgdtw	[eax]
	movs	es:[edi], byte ptr [esi]
	movs	dword ptr es:[edi], [esi]
	movs	word ptr es:[edi], word ptr [esi]
	movsx	eax, byte ptr [eax]
	movsx	eax, word ptr [eax]
	paddb	mm0, [eax]
	paddb	mm0, qword ptr [eax]
	paddb	xmm0, [eax]
	paddb	xmm0, xmmword ptr [eax]
	pinsrw	mm0, word ptr [eax], 3
	pinsrw	xmm0, word ptr [eax], 7
	push	dword ptr [eax]
	xlat	[ebx]
	xlat	byte ptr [ebx]
	xlatb

	# memory operands

	mov	eax, dword ptr [byte+eax]
	mov	eax, dword ptr byte[eax]
	mov	eax, [dword+eax]
	mov	eax, dword[eax]
	mov	eax, [fword+eax]
	mov	eax, fword[eax]
	mov	eax, [qword+eax+dword]
	mov	eax, qword[eax+dword]
	mov	eax, [tbyte+eax+dword*2]
	mov	eax, tbyte[eax+dword*2]
#	mov	eax, [word+eax*dword]
#	mov	eax, word[eax*dword]
#	mov	eax, [xmmword+(eax+1)*dword]
#	mov	eax, xmmword[(eax+1)*dword]

	# expressions

	push	+ 1
	push	- 1
	push	not 1
	push	1 + 1
	push	2 - 1
	push	2 * 2
	push	3 / 2
	push	3 mod 2
	push	4 shl 1
	push	5 shr 2
	push	6 and 3
	push	7 xor 4
	push	8 or 5

	# Force a good alignment.
	.p2align	4,0
