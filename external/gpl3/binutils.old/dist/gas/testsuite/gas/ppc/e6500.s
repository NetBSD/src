# Power E6500 tests
	.text
start:
	vabsdub	0, 1, 2
	vabsduh	0, 1, 2
	vabsduw	0, 1, 2
	mvidsplt 0, 1, 2
	mviwsplt 0, 1, 2
	lvexbx	0, 0, 2
	lvexbx	0, 1, 2
	lvexhx	0, 0, 2
	lvexhx	0, 1, 2
	lvexwx	0, 0, 2
	lvexwx	0, 1, 2
	stvexbx	0, 0, 2
	stvexbx	0, 1, 2
	stvexhx	0, 0, 2
	stvexhx	0, 1, 2
	stvexwx	0, 0, 2
	stvexwx	0, 1, 2
	lvepx	0, 0, 2
	lvepx	0, 1, 2
	lvepxl	0, 0, 2
	lvepxl	0, 1, 2
	stvepx	0, 0, 2
	stvepx	0, 1, 2
	stvepxl	0, 0, 2
	stvepxl	0, 1, 2
	lvtlx	0, 0, 2
	lvtlx	0, 1, 2
	lvtlxl	0, 0, 2
	lvtlxl	0, 1, 2
	lvtrx	0, 0, 2
	lvtrx	0, 1, 2
	lvtrxl	0, 0, 2
	lvtrxl	0, 1, 2
	stvflx	0, 0, 2
	stvflx	0, 1, 2
	stvflxl	0, 0, 2
	stvflxl	0, 1, 2
	stvfrx	0, 0, 2
	stvfrx	0, 1, 2
	stvfrxl	0, 0, 2
	stvfrxl	0, 1, 2
	lvswx	0, 0, 2
	lvswx	0, 1, 2
	lvswxl	0, 0, 2
	lvswxl	0, 1, 2
	stvswx	0, 0, 2
	stvswx	0, 1, 2
	stvswxl	0, 0, 2
	stvswxl	0, 1, 2
	lvsm	0, 0, 2
	lvsm	0, 1, 2
	miso
	sync
	sync	0,0
	sync	1,0
	sync	2,0
	sync	3,7
	sync	3,8
	dni	0,0
	dni	31,31
	dcblq.	2,0,1
	dcblq.	2,3,1
	icblq.	2,0,1
	icblq.	2,3,1
	mftmr	0,16
	mttmr	16,0
