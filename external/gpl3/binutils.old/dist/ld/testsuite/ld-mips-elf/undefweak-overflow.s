# relocs against undefined weak symbols should not be treated as
# overflowing

	.module mips64r6
	.globl	start
	.weak	foo
start:
	.set noreorder
	.set mips64r6
	beqzc	$2, foo
	nop 
	bnezc	$2, foo
	lwpc	$2, foo
	ldpc	$2, foo
	bc	foo
	auipc	$4, %pcrel_hi(foo)
	addiu	$4, $4, %pcrel_lo(foo+4)

	b	foo
	nop
	bal	foo
	lui	$4, %gp_rel(foo)

	.set mips32r2
	.set micromips
micro:
	beqz16	$4, foo
	nop
	b16	foo
	nop
	b	foo
	nop
