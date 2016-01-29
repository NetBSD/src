# Invalid Intel MCU instructions
	.text

	fnstsw
	fstsw	%ax

	cmove	%eax,%ebx
	nopw	(%eax)

	movq	%xmm1, (%eax)
	movnti	%eax, (%eax)
