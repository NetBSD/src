
	.p2align 11
vector_start:
/*
 * Exception taken from current Exception Level with SP_EL0.
 * (These shouldn't happen)
 */
vec_sp_el0_sync:
	hlt	#0 * 0x80
	.p2align 7
vec_sp_el0_irq:
	hlt	#1 * 0x80
	.p2align 7
vec_sp_el0_fiq:
	hlt	#2 * 0x80
	.p2align 7
vec_sp_el0_serror:
	hlt	#3 * 0x80
/*
 * Exception takend form current Exception Level with SP_ELx where x > 0.
 * There are entries for exceptions caused in EL1 (kernel exceptions).
 */
	.p2align 7
vec_sp_el1_sync:
	hlt	#4 * 0x80
	.p2align 7
vec_sp_el1_irq:
	hlt	#5 * 0x80
	.p2align 7
vec_sp_el1_fiq:
	hlt	#6 * 0x80
	.p2align 7
vec_sp_el1_serror:
	hlt	#7 * 0x80
/*
 * Exception taken from lower Exception Level which is using AArch64
 * There are entries for exceptions caused in EL0 (native user exceptions).
 */
	.p2align 7
vec_a64_el0_sync:
	hlt	#8 * 0x80
	.p2align 7
vec_a64_el0_irq:
	hlt	#9 * 0x80
	.p2align 7
vec_a64_el0_fiq:
	hlt	#10 * 0x80
	.p2align 7
vec_a64_el0_serror:
	hlt	#11 * 0x80
/*
 * Exception taken from lower Exception Level which is using AArch32
 * There are entries for exceptions caused in EL0 (compat user exceptions).
 */
	.p2align 7
vec_a32_el0_sync:
	hlt	#12 * 0x80
	.p2align 7
vec_a32_el0_irq:
	hlt	#13 * 0x80
	.p2align 7
vec_a32_el0_fiq:
	hlt	#14 * 0x80
	.p2align 7
vec_a32_el0_serror:
	hlt	#15 * 0x80
	.p2align 7
vector_end:
