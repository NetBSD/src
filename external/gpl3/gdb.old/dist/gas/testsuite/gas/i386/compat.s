# Check SYSV mnemonic instructions.
	.text 
	fsub	%st,%st(3)
	fsubp
	fsubp	%st(3)
	fsubp	%st,%st(3)
	fsubr	%st,%st(3)
	fsubrp
	fsubrp	%st(3)
	fsubrp	%st,%st(3)
	fdiv	%st,%st(3)
	fdivp
	fdivp	%st(3)
	fdivp	%st,%st(3)
	fdivr	%st,%st(3)
	fdivrp
	fdivrp	%st(3)
	fdivrp	%st,%st(3)
