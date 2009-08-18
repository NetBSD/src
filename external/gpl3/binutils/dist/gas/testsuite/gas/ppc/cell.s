	.section	".text"
	lvlx %r0, %r1, %r2
	lvlx %r0, 0, %r2
	lvlxl %r0, %r1, %r2
	lvlxl %r0, 0, %r2
	lvrx %r0, %r1, %r2
	lvrx %r0, 0, %r2
	lvrxl %r0, %r1, %r2
	lvrxl %r0, 0, %r2

	stvlx %r0, %r1, %r2
	stvlx %r0, 0, %r2
	stvlxl %r0, %r1, %r2
	stvlxl %r0, 0, %r2
	stvrx %r0, %r1, %r2
	stvrx %r0, 0, %r2
	stvrxl %r0, %r1, %r2
	stvrxl %r0, 0, %r2

	ldbrx %r0, 0, %r1
	ldbrx %r0, %r1, %r2

	stdbrx %r0, 0, %r1
	stdbrx %r0, %r1, %r2

	dss	3
	dssall	
	dst	5,4,1
	dstt	8,7,0
	dstst	5,6,3
	dststt	4,5,2
