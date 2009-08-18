	.file	"power7.c"
	.section	".text"
	.align 2
	.p2align 4,,15
	.globl power7
	.type	power7, @function
power7:
	lxvd2x    3,4,5
	lxvd2ux   3,4,5
	lxvd2x    43,4,5
	lxvd2ux   43,4,5
	stxvd2x   3,4,5
	stxvd2ux  3,4,5
	stxvd2x   43,4,5
	stxvd2ux  43,4,5
	xxmrghd   3,4,5
	xxmrghd   43,44,45
	xxmrgld   3,4,5
	xxmrgld   43,44,45
	xxpermdi  3,4,5,0
	xxpermdi  43,44,45,0
	xxpermdi  3,4,5,3
	xxpermdi  43,44,45,3
	xxpermdi  3,4,5,1
	xxpermdi  43,44,45,1
	xxpermdi  3,4,5,2
	xxpermdi  43,44,45,2
	xvmovdp   3,4
	xvmovdp   43,44
	xvcpsgndp 3,4,4
	xvcpsgndp 43,44,44
	xvcpsgndp 3,4,5
	xvcpsgndp 43,44,45
	doze
	nap
	sleep
	rvwinkle
	prtyw     3,4
	prtyd     13,14
	mfcfar    10
	mtcfar    11
	cmpb      3,4,5
	mffgpr    6,7
	mftgpr    8,9
	lwzcix    10,11,12
	lfdpx     13,14,15
	dadd      16,17,18
	daddq     20,22,24
	dss       3
	dssall  
	dst       5,4,1
	dstt      8,7,0
	dstst     5,6,3
	dststt    4,5,2
	blr
	.size	power7,.-power7
	.ident	"GCC: (GNU) 4.1.2 20070115 (prerelease) (SUSE Linux)"
	.section	.note.GNU-stack,"",@progbits
