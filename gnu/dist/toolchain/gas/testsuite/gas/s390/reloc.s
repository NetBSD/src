.text
foo:
	mvc	0(test_R_390_8,%r1),0(%r2)
	l	%r0,test_R_390_12(%r1,%r2)
	lhi	%r0,test_R_390_16
	.long	test_R_390_32
	.long	test_R_390_PC32-foo
	l	%r0,test_R_390_GOT12@GOT(%r1,%r2)
	.long	test_R_390_GOT32@GOT
	.long	test_R_390_PLT32@PLT
	lhi	%r0,test_R_390_GOT16@GOT
	lhi	%r0,test_R_390_PC16-foo
	bras	%r14,test_R_390_PC16DBL
	bras	%r14,test_R_390_PLT16DBL
