/*
 * startprog(entry, stack, argc, argv, prom_magic, prom_ptr, bi_magic, bi_ptr);
 *
 * load new stack pointer, then call
 *  entry(argc, argv, prom_magic, prom_ptr, bi_magic, bi_ptr);
 */

#include <mips/asm.h>

LEAF(startprog)
        subu    sp,sp,32
	addu	t0,a1,-CALLFRAME_SIZ	# new stack value
	move	t1,a0
	move	a0,a2
	move	a1,a3

	lw	a2,48(sp)		# load everything from old stack we need
	lw	a3,52(sp)
	lw	v0,56(sp)
	lw	v1,60(sp)

	move	sp,t0			# new stack pointer
	sw	ra,24(sp)

	sw	v0,16(sp)
	sw	v1,20(sp)
	jal	ra,t1
	nop				# BDslot

	lw	ra,24(sp)		# should not get back here!
	j	ra
	addu	sp,sp,32
END(startprog)
