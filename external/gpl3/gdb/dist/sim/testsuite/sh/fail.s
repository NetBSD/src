# sh testcase, fail
# mach:	 all
# as(sh):	-defsym sim_cpu=0
# as(shdsp):	-defsym sim_cpu=1 -dsp 
# output: fail\n
# status: 1

	.include "testutils.inc"

	start
	
	fail

	exit 0

