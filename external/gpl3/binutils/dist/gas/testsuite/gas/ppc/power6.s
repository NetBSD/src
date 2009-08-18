# PowerPC POWER6 AltiVec tests
#as: -mpower6
	.section ".text"
start:
	doze
	nap
	sleep
	rvwinkle
	prtyw	3,4
	prtyd	13,14
	mfcfar	10
	mtcfar	11
	cmpb	3,4,5
	mffgpr	6,7
	mftgpr	8,9
	lwzcix	10,11,12
	lfdpx	13,14,15
	dadd	16,17,18
	daddq	20,22,24
	dss	3
	dssall	
	dst	5,4,1
	dstt	8,7,0
	dstst	5,6,3
	dststt	4,5,2
