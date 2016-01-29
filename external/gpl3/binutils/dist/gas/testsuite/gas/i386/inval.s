	.text
	.allow_index_reg
# All the following should be illegal
	mov	(%dx),%al
	mov	(%eax,%esp,2),%al
	setae	%eax
	pushb	%ds
	popb	%ds
	pushb	%al
	popb	%al
	pushb	%ah
	popb	%ah
	pushb	%ax
	popb	%ax
	pushb	%eax
	popb	%eax
	movb	%ds,%ax
	movb	%ds,%eax
	movb	%ax,%ds
	movb	%eax,%ds
	movdb	%eax,%mm0
	movqb	0,%mm0
	ldsb	0,%eax
	setnew	0
	movdw	%eax,%mm0
	movqw	0,%mm0
	div	%cx,%al
	div	%cl,%ax
	div	%ecx,%al
	imul	10,%bx,%ecx
	imul	10,%bx,%al
	popab
	stil
	aaab
	cwdel
	cwdw
	callww	0
foo:	jaw	foo
	jcxzw	foo
	jecxzl	foo
	loopb	foo
	xlatw	%es:%bx
	xlatl	%es:%bx
	intl	2
	int3b
	hltb
	fstb	%st(0)
	fcompll	28(%ebp)
	fldlw	(%eax)
	movl	$%ebx,%eax
	insertq	$4,$2,%xmm2,%ebx
	cvtsi2ssq (%eax),%xmm1
	cvtsi2sdq (%eax),%xmm1
	fnstsw %eax
	fnstsw %al
	fstsw %eax
	fstsw %al

movnti %ax, (%eax)
movntiw %ax, (%eax)

	add (%si,%esi), %eax
	add (%esi,%si), %eax
	add (%eiz), %eax
	add (%eax), %eiz

	.intel_syntax noprefix
	cvtsi2ss xmm1,QWORD PTR [eax]
	cvtsi2sd xmm1,QWORD PTR [eax]
	cvtsi2ssq xmm1,QWORD PTR [eax]
	cvtsi2sdq xmm1,QWORD PTR [eax]
	movq xmm1, XMMWORD PTR [esp]
	movq xmm1, DWORD PTR [esp]
	movq xmm1, WORD PTR [esp]
	movq xmm1, BYTE PTR [esp]
	movq XMMWORD PTR [esp],xmm1
	movq DWORD PTR [esp],xmm1
	movq WORD PTR [esp],xmm1
	movq BYTE PTR [esp],xmm1
	fnstsw eax
	fnstsw al
	fstsw eax
	fstsw al

movsx ax, [eax]
movsx eax, [eax]
movzx ax, [eax]
movzx eax, [eax]

movnti word ptr [eax], ax
