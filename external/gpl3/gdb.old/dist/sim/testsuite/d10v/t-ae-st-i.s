# mach: all
# output:
# sim: --environment operating

.include "t-macros.i"

	start

	PSW_BITS = 0
	point_dmap_at_imem
	check_interrupt (VEC_AE&DMAP_MASK)+DMAP_BASE PSW_BITS test_st

	ldi r10,#0x4000
	st r8, @r10

	ldi r10,#0x4001
test_st:
	st r8,@r10
	nop
	exit47
