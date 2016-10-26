# Test reads/writes to the %mwait asr register and the MWAIT
# instruction
	.text
	rd	%mwait, %g1
	wr	%g2, 0x3, %mwait
	mwait	%g1
	mwait	0x3
