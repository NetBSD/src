.text
foo:
	brasl	%r14,test_R_390_PC32DBL
	brasl	%r14,test_R_390_PLT32DBL
	.quad	test_R_390_64
	.quad	test_R_390_PC64-foo
	.quad	test_R_390_GOT64@GOT
	.quad	test_R_390_PLT64@PLT
	larl	%r1,test_R_390_GOTENT@GOT
