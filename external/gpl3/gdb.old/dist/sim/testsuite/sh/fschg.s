# sh testcase for fschg
# mach: sh
# as(sh):	-defsym sim_cpu=0

	.include "testutils.inc"

	start
	set_grs_a5a5
	set_fprs_a5a5
	sts	fpscr, r0
	assertreg0	0
	fschg
	sts	fpscr, r0
	assertreg0	0x100000
	fschg
	sts	fpscr, r0
	assertreg0	0
	fschg
	sts	fpscr, r0
	assertreg0	0x100000
	fschg
	sts	fpscr, r0
	assertreg0	0

	set_greg 0xa5a5a5a5 r0
	test_grs_a5a5
	test_fprs_a5a5
	pass
	exit 0
