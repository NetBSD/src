#ifdef __linux__
#include <asm/regdef.h>
#else
#include <asm.h>
#include <regdef.h>
#endif

.text

.set	noat
.set	noreorder
.globl	sha1_block_data_order
.align	5
.ent	sha1_block_data_order
sha1_block_data_order:
	lda	sp,-64(sp)
	stq	ra,0(sp)
	stq	s0,8(sp)
	stq	s1,16(sp)
	stq	s2,24(sp)
	stq	s3,32(sp)
	stq	s4,40(sp)
	stq	s5,48(sp)
	stq	fp,56(sp)
	.mask	0x0400fe00,-64
	.frame	sp,64,ra
	.prologue 0

	ldl	a3,0(a0)
	ldl	a4,4(a0)
	sll	a2,6,a2
	ldl	a5,8(a0)
	ldl	t8,12(a0)
	ldl	t9,16(a0)
	addq	a1,a2,a2

.Lloop:
	.set	noreorder
	ldah	AT,23170(zero)
	zapnot	a4,0xf,a4
	lda	AT,31129(AT)	# K_00_19
	ldq_u	$0,0+0(a1)
	ldq_u	$1,0+7(a1)
	ldq_u	$2,(0+2)*4+0(a1)
	ldq_u	$3,(0+2)*4+7(a1)
	extql	$0,a1,$0
	extqh	$1,a1,$1

	or	$1,$0,$0	# pair of 32-bit values are fetched

	srl	$0,24,t10		# vectorized byte swap
	srl	$0,8,ra

	sll	$0,8,t12
	sll	$0,24,$0
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$0,0x88,$0
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	a3,5,t11

	or	$0,t10,$0
	addl	AT,t9,t9
	and	a4,a5,ra
	zapnot	a3,0xf,a3

	or	$0,t12,$0
	srl	a3,27,t10
	bic	t8,a4,t12
	sll	a4,30,a4

	extll	$0,4,$1	# extract upper half
	or	ra,t12,ra
	addl	$0,t9,t9

	addl	t11,t9,t9
	srl	a4,32,t12
	zapnot	$0,0xf,$0

	addl	t10,t9,t9
	addl	ra,t9,t9
	or	t12,a4,a4
	sll	t9,5,t11
	addl	AT,t8,t8
	and	a3,a4,ra
	zapnot	t9,0xf,t9

	srl	t9,27,t10
	addl	$1,t8,t8
	bic	a5,a3,t12
	sll	a3,30,a3

	or	ra,t12,ra
	addl	t11,t8,t8
	srl	a3,32,t12
	zapnot	$1,0xf,$1

	addl	t10,t8,t8
	addl	ra,t8,t8
	or	t12,a3,a3
	ldq_u	$4,(2+2)*4+0(a1)
	ldq_u	$5,(2+2)*4+7(a1)
	extql	$2,a1,$2
	extqh	$3,a1,$3

	or	$3,$2,$2	# pair of 32-bit values are fetched

	srl	$2,24,t10		# vectorized byte swap
	srl	$2,8,ra

	sll	$2,8,t12
	sll	$2,24,$2
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$2,0x88,$2
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	t8,5,t11

	or	$2,t10,$2
	addl	AT,a5,a5
	and	t9,a3,ra
	zapnot	t8,0xf,t8

	or	$2,t12,$2
	srl	t8,27,t10
	bic	a4,t9,t12
	sll	t9,30,t9

	extll	$2,4,$3	# extract upper half
	or	ra,t12,ra
	addl	$2,a5,a5

	addl	t11,a5,a5
	srl	t9,32,t12
	zapnot	$2,0xf,$2

	addl	t10,a5,a5
	addl	ra,a5,a5
	or	t12,t9,t9
	sll	a5,5,t11
	addl	AT,a4,a4
	and	t8,t9,ra
	zapnot	a5,0xf,a5

	srl	a5,27,t10
	addl	$3,a4,a4
	bic	a3,t8,t12
	sll	t8,30,t8

	or	ra,t12,ra
	addl	t11,a4,a4
	srl	t8,32,t12
	zapnot	$3,0xf,$3

	addl	t10,a4,a4
	addl	ra,a4,a4
	or	t12,t8,t8
	ldq_u	$6,(4+2)*4+0(a1)
	ldq_u	$7,(4+2)*4+7(a1)
	extql	$4,a1,$4
	extqh	$5,a1,$5

	or	$5,$4,$4	# pair of 32-bit values are fetched

	srl	$4,24,t10		# vectorized byte swap
	srl	$4,8,ra

	sll	$4,8,t12
	sll	$4,24,$4
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$4,0x88,$4
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	a4,5,t11

	or	$4,t10,$4
	addl	AT,a3,a3
	and	a5,t8,ra
	zapnot	a4,0xf,a4

	or	$4,t12,$4
	srl	a4,27,t10
	bic	t9,a5,t12
	sll	a5,30,a5

	extll	$4,4,$5	# extract upper half
	or	ra,t12,ra
	addl	$4,a3,a3

	addl	t11,a3,a3
	srl	a5,32,t12
	zapnot	$4,0xf,$4

	addl	t10,a3,a3
	addl	ra,a3,a3
	or	t12,a5,a5
	sll	a3,5,t11
	addl	AT,t9,t9
	and	a4,a5,ra
	zapnot	a3,0xf,a3

	srl	a3,27,t10
	addl	$5,t9,t9
	bic	t8,a4,t12
	sll	a4,30,a4

	or	ra,t12,ra
	addl	t11,t9,t9
	srl	a4,32,t12
	zapnot	$5,0xf,$5

	addl	t10,t9,t9
	addl	ra,t9,t9
	or	t12,a4,a4
	ldq_u	$8,(6+2)*4+0(a1)
	ldq_u	$9,(6+2)*4+7(a1)
	extql	$6,a1,$6
	extqh	$7,a1,$7

	or	$7,$6,$6	# pair of 32-bit values are fetched

	srl	$6,24,t10		# vectorized byte swap
	srl	$6,8,ra

	sll	$6,8,t12
	sll	$6,24,$6
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$6,0x88,$6
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	t9,5,t11

	or	$6,t10,$6
	addl	AT,t8,t8
	and	a3,a4,ra
	zapnot	t9,0xf,t9

	or	$6,t12,$6
	srl	t9,27,t10
	bic	a5,a3,t12
	sll	a3,30,a3

	extll	$6,4,$7	# extract upper half
	or	ra,t12,ra
	addl	$6,t8,t8

	addl	t11,t8,t8
	srl	a3,32,t12
	zapnot	$6,0xf,$6

	addl	t10,t8,t8
	addl	ra,t8,t8
	or	t12,a3,a3
	sll	t8,5,t11
	addl	AT,a5,a5
	and	t9,a3,ra
	zapnot	t8,0xf,t8

	srl	t8,27,t10
	addl	$7,a5,a5
	bic	a4,t9,t12
	sll	t9,30,t9

	or	ra,t12,ra
	addl	t11,a5,a5
	srl	t9,32,t12
	zapnot	$7,0xf,$7

	addl	t10,a5,a5
	addl	ra,a5,a5
	or	t12,t9,t9
	ldq_u	$10,(8+2)*4+0(a1)
	ldq_u	$11,(8+2)*4+7(a1)
	extql	$8,a1,$8
	extqh	$9,a1,$9

	or	$9,$8,$8	# pair of 32-bit values are fetched

	srl	$8,24,t10		# vectorized byte swap
	srl	$8,8,ra

	sll	$8,8,t12
	sll	$8,24,$8
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$8,0x88,$8
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	a5,5,t11

	or	$8,t10,$8
	addl	AT,a4,a4
	and	t8,t9,ra
	zapnot	a5,0xf,a5

	or	$8,t12,$8
	srl	a5,27,t10
	bic	a3,t8,t12
	sll	t8,30,t8

	extll	$8,4,$9	# extract upper half
	or	ra,t12,ra
	addl	$8,a4,a4

	addl	t11,a4,a4
	srl	t8,32,t12
	zapnot	$8,0xf,$8

	addl	t10,a4,a4
	addl	ra,a4,a4
	or	t12,t8,t8
	sll	a4,5,t11
	addl	AT,a3,a3
	and	a5,t8,ra
	zapnot	a4,0xf,a4

	srl	a4,27,t10
	addl	$9,a3,a3
	bic	t9,a5,t12
	sll	a5,30,a5

	or	ra,t12,ra
	addl	t11,a3,a3
	srl	a5,32,t12
	zapnot	$9,0xf,$9

	addl	t10,a3,a3
	addl	ra,a3,a3
	or	t12,a5,a5
	ldq_u	$12,(10+2)*4+0(a1)
	ldq_u	$13,(10+2)*4+7(a1)
	extql	$10,a1,$10
	extqh	$11,a1,$11

	or	$11,$10,$10	# pair of 32-bit values are fetched

	srl	$10,24,t10		# vectorized byte swap
	srl	$10,8,ra

	sll	$10,8,t12
	sll	$10,24,$10
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$10,0x88,$10
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	a3,5,t11

	or	$10,t10,$10
	addl	AT,t9,t9
	and	a4,a5,ra
	zapnot	a3,0xf,a3

	or	$10,t12,$10
	srl	a3,27,t10
	bic	t8,a4,t12
	sll	a4,30,a4

	extll	$10,4,$11	# extract upper half
	or	ra,t12,ra
	addl	$10,t9,t9

	addl	t11,t9,t9
	srl	a4,32,t12
	zapnot	$10,0xf,$10

	addl	t10,t9,t9
	addl	ra,t9,t9
	or	t12,a4,a4
	sll	t9,5,t11
	addl	AT,t8,t8
	and	a3,a4,ra
	zapnot	t9,0xf,t9

	srl	t9,27,t10
	addl	$11,t8,t8
	bic	a5,a3,t12
	sll	a3,30,a3

	or	ra,t12,ra
	addl	t11,t8,t8
	srl	a3,32,t12
	zapnot	$11,0xf,$11

	addl	t10,t8,t8
	addl	ra,t8,t8
	or	t12,a3,a3
	ldq_u	$14,(12+2)*4+0(a1)
	ldq_u	$15,(12+2)*4+7(a1)
	extql	$12,a1,$12
	extqh	$13,a1,$13

	or	$13,$12,$12	# pair of 32-bit values are fetched

	srl	$12,24,t10		# vectorized byte swap
	srl	$12,8,ra

	sll	$12,8,t12
	sll	$12,24,$12
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$12,0x88,$12
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	t8,5,t11

	or	$12,t10,$12
	addl	AT,a5,a5
	and	t9,a3,ra
	zapnot	t8,0xf,t8

	or	$12,t12,$12
	srl	t8,27,t10
	bic	a4,t9,t12
	sll	t9,30,t9

	extll	$12,4,$13	# extract upper half
	or	ra,t12,ra
	addl	$12,a5,a5

	addl	t11,a5,a5
	srl	t9,32,t12
	zapnot	$12,0xf,$12

	addl	t10,a5,a5
	addl	ra,a5,a5
	or	t12,t9,t9
	sll	a5,5,t11
	addl	AT,a4,a4
	and	t8,t9,ra
	zapnot	a5,0xf,a5

	srl	a5,27,t10
	addl	$13,a4,a4
	bic	a3,t8,t12
	sll	t8,30,t8

	or	ra,t12,ra
	addl	t11,a4,a4
	srl	t8,32,t12
	zapnot	$13,0xf,$13

	addl	t10,a4,a4
	addl	ra,a4,a4
	or	t12,t8,t8
	extql	$14,a1,$14
	extqh	$15,a1,$15

	or	$15,$14,$14	# pair of 32-bit values are fetched

	srl	$14,24,t10		# vectorized byte swap
	srl	$14,8,ra

	sll	$14,8,t12
	sll	$14,24,$14
	zapnot	t10,0x11,t10
	zapnot	ra,0x22,ra

	zapnot	$14,0x88,$14
	or	t10,ra,t10
	zapnot	t12,0x44,t12
	sll	a4,5,t11

	or	$14,t10,$14
	addl	AT,a3,a3
	and	a5,t8,ra
	zapnot	a4,0xf,a4

	or	$14,t12,$14
	srl	a4,27,t10
	bic	t9,a5,t12
	sll	a5,30,a5

	extll	$14,4,$15	# extract upper half
	or	ra,t12,ra
	addl	$14,a3,a3

	addl	t11,a3,a3
	srl	a5,32,t12
	zapnot	$14,0xf,$14

	addl	t10,a3,a3
	addl	ra,a3,a3
	or	t12,a5,a5
	sll	a3,5,t11
	addl	AT,t9,t9
	and	a4,a5,ra
	xor	$2,$0,$0

	zapnot	a3,0xf,a3
	addl	$15,t9,t9
	bic	t8,a4,t12
	xor	$8,$0,$0

	srl	a3,27,t10
	addl	t11,t9,t9
	or	ra,t12,ra
	xor	$13,$0,$0

	sll	a4,30,a4
	addl	t10,t9,t9
	srl	$0,31,t11

	addl	ra,t9,t9
	srl	a4,32,t12
	addl	$0,$0,$0

	or	t12,a4,a4
	zapnot	$15,0xf,$15
	or	t11,$0,$0
	sll	t9,5,t11
	addl	AT,t8,t8
	and	a3,a4,ra
	xor	$3,$1,$1

	zapnot	t9,0xf,t9
	addl	$0,t8,t8
	bic	a5,a3,t12
	xor	$9,$1,$1

	srl	t9,27,t10
	addl	t11,t8,t8
	or	ra,t12,ra
	xor	$14,$1,$1

	sll	a3,30,a3
	addl	t10,t8,t8
	srl	$1,31,t11

	addl	ra,t8,t8
	srl	a3,32,t12
	addl	$1,$1,$1

	or	t12,a3,a3
	zapnot	$0,0xf,$0
	or	t11,$1,$1
	sll	t8,5,t11
	addl	AT,a5,a5
	and	t9,a3,ra
	xor	$4,$2,$2

	zapnot	t8,0xf,t8
	addl	$1,a5,a5
	bic	a4,t9,t12
	xor	$10,$2,$2

	srl	t8,27,t10
	addl	t11,a5,a5
	or	ra,t12,ra
	xor	$15,$2,$2

	sll	t9,30,t9
	addl	t10,a5,a5
	srl	$2,31,t11

	addl	ra,a5,a5
	srl	t9,32,t12
	addl	$2,$2,$2

	or	t12,t9,t9
	zapnot	$1,0xf,$1
	or	t11,$2,$2
	sll	a5,5,t11
	addl	AT,a4,a4
	and	t8,t9,ra
	xor	$5,$3,$3

	zapnot	a5,0xf,a5
	addl	$2,a4,a4
	bic	a3,t8,t12
	xor	$11,$3,$3

	srl	a5,27,t10
	addl	t11,a4,a4
	or	ra,t12,ra
	xor	$0,$3,$3

	sll	t8,30,t8
	addl	t10,a4,a4
	srl	$3,31,t11

	addl	ra,a4,a4
	srl	t8,32,t12
	addl	$3,$3,$3

	or	t12,t8,t8
	zapnot	$2,0xf,$2
	or	t11,$3,$3
	sll	a4,5,t11
	addl	AT,a3,a3
	and	a5,t8,ra
	xor	$6,$4,$4

	zapnot	a4,0xf,a4
	addl	$3,a3,a3
	bic	t9,a5,t12
	xor	$12,$4,$4

	srl	a4,27,t10
	addl	t11,a3,a3
	or	ra,t12,ra
	xor	$1,$4,$4

	sll	a5,30,a5
	addl	t10,a3,a3
	srl	$4,31,t11

	addl	ra,a3,a3
	srl	a5,32,t12
	addl	$4,$4,$4

	or	t12,a5,a5
	zapnot	$3,0xf,$3
	or	t11,$4,$4
	ldah	AT,28378(zero)
	lda	AT,-5215(AT)	# K_20_39
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$7,$5,$5

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$13,$5,$5

	srl	a4,2,a4
	addl	$4,t9,t9
	xor	t8,ra,ra
	xor	$2,$5,$5

	srl	$5,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$5,$5,$5

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$5,$5
	zapnot	$4,0xf,$4
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$8,$6,$6

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$14,$6,$6

	srl	a3,2,a3
	addl	$5,t8,t8
	xor	a5,ra,ra
	xor	$3,$6,$6

	srl	$6,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$6,$6,$6

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$6,$6
	zapnot	$5,0xf,$5
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$9,$7,$7

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$15,$7,$7

	srl	t9,2,t9
	addl	$6,a5,a5
	xor	a4,ra,ra
	xor	$4,$7,$7

	srl	$7,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$7,$7,$7

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$7,$7
	zapnot	$6,0xf,$6
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$10,$8,$8

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$0,$8,$8

	srl	t8,2,t8
	addl	$7,a4,a4
	xor	a3,ra,ra
	xor	$5,$8,$8

	srl	$8,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$8,$8,$8

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$8,$8
	zapnot	$7,0xf,$7
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$11,$9,$9

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	xor	$1,$9,$9

	srl	a5,2,a5
	addl	$8,a3,a3
	xor	t9,ra,ra
	xor	$6,$9,$9

	srl	$9,31,t11
	addl	ra,a3,a3
	srl	a4,27,t10
	addl	$9,$9,$9

	or	t12,a5,a5
	addl	t10,a3,a3
	or	t11,$9,$9
	zapnot	$8,0xf,$8
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$12,$10,$10

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$2,$10,$10

	srl	a4,2,a4
	addl	$9,t9,t9
	xor	t8,ra,ra
	xor	$7,$10,$10

	srl	$10,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$10,$10,$10

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$10,$10
	zapnot	$9,0xf,$9
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$13,$11,$11

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$3,$11,$11

	srl	a3,2,a3
	addl	$10,t8,t8
	xor	a5,ra,ra
	xor	$8,$11,$11

	srl	$11,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$11,$11,$11

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$11,$11
	zapnot	$10,0xf,$10
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$14,$12,$12

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$4,$12,$12

	srl	t9,2,t9
	addl	$11,a5,a5
	xor	a4,ra,ra
	xor	$9,$12,$12

	srl	$12,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$12,$12,$12

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$12,$12
	zapnot	$11,0xf,$11
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$15,$13,$13

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$5,$13,$13

	srl	t8,2,t8
	addl	$12,a4,a4
	xor	a3,ra,ra
	xor	$10,$13,$13

	srl	$13,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$13,$13,$13

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$13,$13
	zapnot	$12,0xf,$12
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$0,$14,$14

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	xor	$6,$14,$14

	srl	a5,2,a5
	addl	$13,a3,a3
	xor	t9,ra,ra
	xor	$11,$14,$14

	srl	$14,31,t11
	addl	ra,a3,a3
	srl	a4,27,t10
	addl	$14,$14,$14

	or	t12,a5,a5
	addl	t10,a3,a3
	or	t11,$14,$14
	zapnot	$13,0xf,$13
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$1,$15,$15

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$7,$15,$15

	srl	a4,2,a4
	addl	$14,t9,t9
	xor	t8,ra,ra
	xor	$12,$15,$15

	srl	$15,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$15,$15,$15

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$15,$15
	zapnot	$14,0xf,$14
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$2,$0,$0

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$8,$0,$0

	srl	a3,2,a3
	addl	$15,t8,t8
	xor	a5,ra,ra
	xor	$13,$0,$0

	srl	$0,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$0,$0,$0

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$0,$0
	zapnot	$15,0xf,$15
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$3,$1,$1

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$9,$1,$1

	srl	t9,2,t9
	addl	$0,a5,a5
	xor	a4,ra,ra
	xor	$14,$1,$1

	srl	$1,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$1,$1,$1

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$1,$1
	zapnot	$0,0xf,$0
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$4,$2,$2

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$10,$2,$2

	srl	t8,2,t8
	addl	$1,a4,a4
	xor	a3,ra,ra
	xor	$15,$2,$2

	srl	$2,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$2,$2,$2

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$2,$2
	zapnot	$1,0xf,$1
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$5,$3,$3

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	xor	$11,$3,$3

	srl	a5,2,a5
	addl	$2,a3,a3
	xor	t9,ra,ra
	xor	$0,$3,$3

	srl	$3,31,t11
	addl	ra,a3,a3
	srl	a4,27,t10
	addl	$3,$3,$3

	or	t12,a5,a5
	addl	t10,a3,a3
	or	t11,$3,$3
	zapnot	$2,0xf,$2
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$6,$4,$4

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$12,$4,$4

	srl	a4,2,a4
	addl	$3,t9,t9
	xor	t8,ra,ra
	xor	$1,$4,$4

	srl	$4,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$4,$4,$4

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$4,$4
	zapnot	$3,0xf,$3
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$7,$5,$5

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$13,$5,$5

	srl	a3,2,a3
	addl	$4,t8,t8
	xor	a5,ra,ra
	xor	$2,$5,$5

	srl	$5,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$5,$5,$5

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$5,$5
	zapnot	$4,0xf,$4
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$8,$6,$6

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$14,$6,$6

	srl	t9,2,t9
	addl	$5,a5,a5
	xor	a4,ra,ra
	xor	$3,$6,$6

	srl	$6,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$6,$6,$6

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$6,$6
	zapnot	$5,0xf,$5
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$9,$7,$7

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$15,$7,$7

	srl	t8,2,t8
	addl	$6,a4,a4
	xor	a3,ra,ra
	xor	$4,$7,$7

	srl	$7,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$7,$7,$7

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$7,$7
	zapnot	$6,0xf,$6
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$10,$8,$8

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	xor	$0,$8,$8

	srl	a5,2,a5
	addl	$7,a3,a3
	xor	t9,ra,ra
	xor	$5,$8,$8

	srl	$8,31,t11
	addl	ra,a3,a3
	srl	a4,27,t10
	addl	$8,$8,$8

	or	t12,a5,a5
	addl	t10,a3,a3
	or	t11,$8,$8
	zapnot	$7,0xf,$7
	ldah	AT,-28900(zero)
	lda	AT,-17188(AT)	# K_40_59
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$11,$9,$9

	srl	a3,27,t10
	and	a4,a5,ra
	and	a4,t8,t12
	xor	$1,$9,$9

	sll	a4,30,a4
	addl	t11,t9,t9
	xor	$6,$9,$9

	srl	$9,31,t11
	addl	t10,t9,t9
	or	ra,t12,ra
	and	a5,t8,t12

	or	ra,t12,ra
	srl	a4,32,t12
	addl	$8,t9,t9
	addl	$9,$9,$9

	or	t12,a4,a4
	addl	ra,t9,t9
	or	t11,$9,$9
	zapnot	$8,0xf,$8
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$12,$10,$10

	srl	t9,27,t10
	and	a3,a4,ra
	and	a3,a5,t12
	xor	$2,$10,$10

	sll	a3,30,a3
	addl	t11,t8,t8
	xor	$7,$10,$10

	srl	$10,31,t11
	addl	t10,t8,t8
	or	ra,t12,ra
	and	a4,a5,t12

	or	ra,t12,ra
	srl	a3,32,t12
	addl	$9,t8,t8
	addl	$10,$10,$10

	or	t12,a3,a3
	addl	ra,t8,t8
	or	t11,$10,$10
	zapnot	$9,0xf,$9
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$13,$11,$11

	srl	t8,27,t10
	and	t9,a3,ra
	and	t9,a4,t12
	xor	$3,$11,$11

	sll	t9,30,t9
	addl	t11,a5,a5
	xor	$8,$11,$11

	srl	$11,31,t11
	addl	t10,a5,a5
	or	ra,t12,ra
	and	a3,a4,t12

	or	ra,t12,ra
	srl	t9,32,t12
	addl	$10,a5,a5
	addl	$11,$11,$11

	or	t12,t9,t9
	addl	ra,a5,a5
	or	t11,$11,$11
	zapnot	$10,0xf,$10
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$14,$12,$12

	srl	a5,27,t10
	and	t8,t9,ra
	and	t8,a3,t12
	xor	$4,$12,$12

	sll	t8,30,t8
	addl	t11,a4,a4
	xor	$9,$12,$12

	srl	$12,31,t11
	addl	t10,a4,a4
	or	ra,t12,ra
	and	t9,a3,t12

	or	ra,t12,ra
	srl	t8,32,t12
	addl	$11,a4,a4
	addl	$12,$12,$12

	or	t12,t8,t8
	addl	ra,a4,a4
	or	t11,$12,$12
	zapnot	$11,0xf,$11
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$15,$13,$13

	srl	a4,27,t10
	and	a5,t8,ra
	and	a5,t9,t12
	xor	$5,$13,$13

	sll	a5,30,a5
	addl	t11,a3,a3
	xor	$10,$13,$13

	srl	$13,31,t11
	addl	t10,a3,a3
	or	ra,t12,ra
	and	t8,t9,t12

	or	ra,t12,ra
	srl	a5,32,t12
	addl	$12,a3,a3
	addl	$13,$13,$13

	or	t12,a5,a5
	addl	ra,a3,a3
	or	t11,$13,$13
	zapnot	$12,0xf,$12
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$0,$14,$14

	srl	a3,27,t10
	and	a4,a5,ra
	and	a4,t8,t12
	xor	$6,$14,$14

	sll	a4,30,a4
	addl	t11,t9,t9
	xor	$11,$14,$14

	srl	$14,31,t11
	addl	t10,t9,t9
	or	ra,t12,ra
	and	a5,t8,t12

	or	ra,t12,ra
	srl	a4,32,t12
	addl	$13,t9,t9
	addl	$14,$14,$14

	or	t12,a4,a4
	addl	ra,t9,t9
	or	t11,$14,$14
	zapnot	$13,0xf,$13
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$1,$15,$15

	srl	t9,27,t10
	and	a3,a4,ra
	and	a3,a5,t12
	xor	$7,$15,$15

	sll	a3,30,a3
	addl	t11,t8,t8
	xor	$12,$15,$15

	srl	$15,31,t11
	addl	t10,t8,t8
	or	ra,t12,ra
	and	a4,a5,t12

	or	ra,t12,ra
	srl	a3,32,t12
	addl	$14,t8,t8
	addl	$15,$15,$15

	or	t12,a3,a3
	addl	ra,t8,t8
	or	t11,$15,$15
	zapnot	$14,0xf,$14
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$2,$0,$0

	srl	t8,27,t10
	and	t9,a3,ra
	and	t9,a4,t12
	xor	$8,$0,$0

	sll	t9,30,t9
	addl	t11,a5,a5
	xor	$13,$0,$0

	srl	$0,31,t11
	addl	t10,a5,a5
	or	ra,t12,ra
	and	a3,a4,t12

	or	ra,t12,ra
	srl	t9,32,t12
	addl	$15,a5,a5
	addl	$0,$0,$0

	or	t12,t9,t9
	addl	ra,a5,a5
	or	t11,$0,$0
	zapnot	$15,0xf,$15
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$3,$1,$1

	srl	a5,27,t10
	and	t8,t9,ra
	and	t8,a3,t12
	xor	$9,$1,$1

	sll	t8,30,t8
	addl	t11,a4,a4
	xor	$14,$1,$1

	srl	$1,31,t11
	addl	t10,a4,a4
	or	ra,t12,ra
	and	t9,a3,t12

	or	ra,t12,ra
	srl	t8,32,t12
	addl	$0,a4,a4
	addl	$1,$1,$1

	or	t12,t8,t8
	addl	ra,a4,a4
	or	t11,$1,$1
	zapnot	$0,0xf,$0
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$4,$2,$2

	srl	a4,27,t10
	and	a5,t8,ra
	and	a5,t9,t12
	xor	$10,$2,$2

	sll	a5,30,a5
	addl	t11,a3,a3
	xor	$15,$2,$2

	srl	$2,31,t11
	addl	t10,a3,a3
	or	ra,t12,ra
	and	t8,t9,t12

	or	ra,t12,ra
	srl	a5,32,t12
	addl	$1,a3,a3
	addl	$2,$2,$2

	or	t12,a5,a5
	addl	ra,a3,a3
	or	t11,$2,$2
	zapnot	$1,0xf,$1
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$5,$3,$3

	srl	a3,27,t10
	and	a4,a5,ra
	and	a4,t8,t12
	xor	$11,$3,$3

	sll	a4,30,a4
	addl	t11,t9,t9
	xor	$0,$3,$3

	srl	$3,31,t11
	addl	t10,t9,t9
	or	ra,t12,ra
	and	a5,t8,t12

	or	ra,t12,ra
	srl	a4,32,t12
	addl	$2,t9,t9
	addl	$3,$3,$3

	or	t12,a4,a4
	addl	ra,t9,t9
	or	t11,$3,$3
	zapnot	$2,0xf,$2
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$6,$4,$4

	srl	t9,27,t10
	and	a3,a4,ra
	and	a3,a5,t12
	xor	$12,$4,$4

	sll	a3,30,a3
	addl	t11,t8,t8
	xor	$1,$4,$4

	srl	$4,31,t11
	addl	t10,t8,t8
	or	ra,t12,ra
	and	a4,a5,t12

	or	ra,t12,ra
	srl	a3,32,t12
	addl	$3,t8,t8
	addl	$4,$4,$4

	or	t12,a3,a3
	addl	ra,t8,t8
	or	t11,$4,$4
	zapnot	$3,0xf,$3
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$7,$5,$5

	srl	t8,27,t10
	and	t9,a3,ra
	and	t9,a4,t12
	xor	$13,$5,$5

	sll	t9,30,t9
	addl	t11,a5,a5
	xor	$2,$5,$5

	srl	$5,31,t11
	addl	t10,a5,a5
	or	ra,t12,ra
	and	a3,a4,t12

	or	ra,t12,ra
	srl	t9,32,t12
	addl	$4,a5,a5
	addl	$5,$5,$5

	or	t12,t9,t9
	addl	ra,a5,a5
	or	t11,$5,$5
	zapnot	$4,0xf,$4
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$8,$6,$6

	srl	a5,27,t10
	and	t8,t9,ra
	and	t8,a3,t12
	xor	$14,$6,$6

	sll	t8,30,t8
	addl	t11,a4,a4
	xor	$3,$6,$6

	srl	$6,31,t11
	addl	t10,a4,a4
	or	ra,t12,ra
	and	t9,a3,t12

	or	ra,t12,ra
	srl	t8,32,t12
	addl	$5,a4,a4
	addl	$6,$6,$6

	or	t12,t8,t8
	addl	ra,a4,a4
	or	t11,$6,$6
	zapnot	$5,0xf,$5
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$9,$7,$7

	srl	a4,27,t10
	and	a5,t8,ra
	and	a5,t9,t12
	xor	$15,$7,$7

	sll	a5,30,a5
	addl	t11,a3,a3
	xor	$4,$7,$7

	srl	$7,31,t11
	addl	t10,a3,a3
	or	ra,t12,ra
	and	t8,t9,t12

	or	ra,t12,ra
	srl	a5,32,t12
	addl	$6,a3,a3
	addl	$7,$7,$7

	or	t12,a5,a5
	addl	ra,a3,a3
	or	t11,$7,$7
	zapnot	$6,0xf,$6
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$10,$8,$8

	srl	a3,27,t10
	and	a4,a5,ra
	and	a4,t8,t12
	xor	$0,$8,$8

	sll	a4,30,a4
	addl	t11,t9,t9
	xor	$5,$8,$8

	srl	$8,31,t11
	addl	t10,t9,t9
	or	ra,t12,ra
	and	a5,t8,t12

	or	ra,t12,ra
	srl	a4,32,t12
	addl	$7,t9,t9
	addl	$8,$8,$8

	or	t12,a4,a4
	addl	ra,t9,t9
	or	t11,$8,$8
	zapnot	$7,0xf,$7
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$11,$9,$9

	srl	t9,27,t10
	and	a3,a4,ra
	and	a3,a5,t12
	xor	$1,$9,$9

	sll	a3,30,a3
	addl	t11,t8,t8
	xor	$6,$9,$9

	srl	$9,31,t11
	addl	t10,t8,t8
	or	ra,t12,ra
	and	a4,a5,t12

	or	ra,t12,ra
	srl	a3,32,t12
	addl	$8,t8,t8
	addl	$9,$9,$9

	or	t12,a3,a3
	addl	ra,t8,t8
	or	t11,$9,$9
	zapnot	$8,0xf,$8
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$12,$10,$10

	srl	t8,27,t10
	and	t9,a3,ra
	and	t9,a4,t12
	xor	$2,$10,$10

	sll	t9,30,t9
	addl	t11,a5,a5
	xor	$7,$10,$10

	srl	$10,31,t11
	addl	t10,a5,a5
	or	ra,t12,ra
	and	a3,a4,t12

	or	ra,t12,ra
	srl	t9,32,t12
	addl	$9,a5,a5
	addl	$10,$10,$10

	or	t12,t9,t9
	addl	ra,a5,a5
	or	t11,$10,$10
	zapnot	$9,0xf,$9
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$13,$11,$11

	srl	a5,27,t10
	and	t8,t9,ra
	and	t8,a3,t12
	xor	$3,$11,$11

	sll	t8,30,t8
	addl	t11,a4,a4
	xor	$8,$11,$11

	srl	$11,31,t11
	addl	t10,a4,a4
	or	ra,t12,ra
	and	t9,a3,t12

	or	ra,t12,ra
	srl	t8,32,t12
	addl	$10,a4,a4
	addl	$11,$11,$11

	or	t12,t8,t8
	addl	ra,a4,a4
	or	t11,$11,$11
	zapnot	$10,0xf,$10
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$14,$12,$12

	srl	a4,27,t10
	and	a5,t8,ra
	and	a5,t9,t12
	xor	$4,$12,$12

	sll	a5,30,a5
	addl	t11,a3,a3
	xor	$9,$12,$12

	srl	$12,31,t11
	addl	t10,a3,a3
	or	ra,t12,ra
	and	t8,t9,t12

	or	ra,t12,ra
	srl	a5,32,t12
	addl	$11,a3,a3
	addl	$12,$12,$12

	or	t12,a5,a5
	addl	ra,a3,a3
	or	t11,$12,$12
	zapnot	$11,0xf,$11
	ldah	AT,-13725(zero)
	lda	AT,-15914(AT)	# K_60_79
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$15,$13,$13

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$5,$13,$13

	srl	a4,2,a4
	addl	$12,t9,t9
	xor	t8,ra,ra
	xor	$10,$13,$13

	srl	$13,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$13,$13,$13

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$13,$13
	zapnot	$12,0xf,$12
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$0,$14,$14

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$6,$14,$14

	srl	a3,2,a3
	addl	$13,t8,t8
	xor	a5,ra,ra
	xor	$11,$14,$14

	srl	$14,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$14,$14,$14

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$14,$14
	zapnot	$13,0xf,$13
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$1,$15,$15

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$7,$15,$15

	srl	t9,2,t9
	addl	$14,a5,a5
	xor	a4,ra,ra
	xor	$12,$15,$15

	srl	$15,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$15,$15,$15

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$15,$15
	zapnot	$14,0xf,$14
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$2,$0,$0

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$8,$0,$0

	srl	t8,2,t8
	addl	$15,a4,a4
	xor	a3,ra,ra
	xor	$13,$0,$0

	srl	$0,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$0,$0,$0

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$0,$0
	zapnot	$15,0xf,$15
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$3,$1,$1

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	xor	$9,$1,$1

	srl	a5,2,a5
	addl	$0,a3,a3
	xor	t9,ra,ra
	xor	$14,$1,$1

	srl	$1,31,t11
	addl	ra,a3,a3
	srl	a4,27,t10
	addl	$1,$1,$1

	or	t12,a5,a5
	addl	t10,a3,a3
	or	t11,$1,$1
	zapnot	$0,0xf,$0
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$4,$2,$2

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$10,$2,$2

	srl	a4,2,a4
	addl	$1,t9,t9
	xor	t8,ra,ra
	xor	$15,$2,$2

	srl	$2,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$2,$2,$2

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$2,$2
	zapnot	$1,0xf,$1
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$5,$3,$3

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$11,$3,$3

	srl	a3,2,a3
	addl	$2,t8,t8
	xor	a5,ra,ra
	xor	$0,$3,$3

	srl	$3,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$3,$3,$3

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$3,$3
	zapnot	$2,0xf,$2
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$6,$4,$4

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$12,$4,$4

	srl	t9,2,t9
	addl	$3,a5,a5
	xor	a4,ra,ra
	xor	$1,$4,$4

	srl	$4,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$4,$4,$4

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$4,$4
	zapnot	$3,0xf,$3
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$7,$5,$5

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$13,$5,$5

	srl	t8,2,t8
	addl	$4,a4,a4
	xor	a3,ra,ra
	xor	$2,$5,$5

	srl	$5,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$5,$5,$5

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$5,$5
	zapnot	$4,0xf,$4
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$8,$6,$6

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	xor	$14,$6,$6

	srl	a5,2,a5
	addl	$5,a3,a3
	xor	t9,ra,ra
	xor	$3,$6,$6

	srl	$6,31,t11
	addl	ra,a3,a3
	srl	a4,27,t10
	addl	$6,$6,$6

	or	t12,a5,a5
	addl	t10,a3,a3
	or	t11,$6,$6
	zapnot	$5,0xf,$5
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$9,$7,$7

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$15,$7,$7

	srl	a4,2,a4
	addl	$6,t9,t9
	xor	t8,ra,ra
	xor	$4,$7,$7

	srl	$7,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$7,$7,$7

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$7,$7
	zapnot	$6,0xf,$6
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$10,$8,$8

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$0,$8,$8

	srl	a3,2,a3
	addl	$7,t8,t8
	xor	a5,ra,ra
	xor	$5,$8,$8

	srl	$8,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$8,$8,$8

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$8,$8
	zapnot	$7,0xf,$7
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$11,$9,$9

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$1,$9,$9

	srl	t9,2,t9
	addl	$8,a5,a5
	xor	a4,ra,ra
	xor	$6,$9,$9

	srl	$9,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$9,$9,$9

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$9,$9
	zapnot	$8,0xf,$8
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$12,$10,$10

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$2,$10,$10

	srl	t8,2,t8
	addl	$9,a4,a4
	xor	a3,ra,ra
	xor	$7,$10,$10

	srl	$10,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$10,$10,$10

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$10,$10
	zapnot	$9,0xf,$9
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	xor	$13,$11,$11

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	xor	$3,$11,$11

	srl	a5,2,a5
	addl	$10,a3,a3
	xor	t9,ra,ra
	xor	$8,$11,$11

	srl	$11,31,t11
	addl	ra,a3,a3
	srl	a4,27,t10
	addl	$11,$11,$11

	or	t12,a5,a5
	addl	t10,a3,a3
	or	t11,$11,$11
	zapnot	$10,0xf,$10
	sll	a3,5,t11
	addl	AT,t9,t9
	zapnot	a3,0xf,a3
	xor	$14,$12,$12

	sll	a4,30,t12
	addl	t11,t9,t9
	xor	a4,a5,ra
	xor	$4,$12,$12

	srl	a4,2,a4
	addl	$11,t9,t9
	xor	t8,ra,ra
	xor	$9,$12,$12

	srl	$12,31,t11
	addl	ra,t9,t9
	srl	a3,27,t10
	addl	$12,$12,$12

	or	t12,a4,a4
	addl	t10,t9,t9
	or	t11,$12,$12
	zapnot	$11,0xf,$11
	sll	t9,5,t11
	addl	AT,t8,t8
	zapnot	t9,0xf,t9
	xor	$15,$13,$13

	sll	a3,30,t12
	addl	t11,t8,t8
	xor	a3,a4,ra
	xor	$5,$13,$13

	srl	a3,2,a3
	addl	$12,t8,t8
	xor	a5,ra,ra
	xor	$10,$13,$13

	srl	$13,31,t11
	addl	ra,t8,t8
	srl	t9,27,t10
	addl	$13,$13,$13

	or	t12,a3,a3
	addl	t10,t8,t8
	or	t11,$13,$13
	zapnot	$12,0xf,$12
	sll	t8,5,t11
	addl	AT,a5,a5
	zapnot	t8,0xf,t8
	xor	$0,$14,$14

	sll	t9,30,t12
	addl	t11,a5,a5
	xor	t9,a3,ra
	xor	$6,$14,$14

	srl	t9,2,t9
	addl	$13,a5,a5
	xor	a4,ra,ra
	xor	$11,$14,$14

	srl	$14,31,t11
	addl	ra,a5,a5
	srl	t8,27,t10
	addl	$14,$14,$14

	or	t12,t9,t9
	addl	t10,a5,a5
	or	t11,$14,$14
	sll	a5,5,t11
	addl	AT,a4,a4
	zapnot	a5,0xf,a5
	xor	$1,$15,$15

	sll	t8,30,t12
	addl	t11,a4,a4
	xor	t8,t9,ra
	xor	$7,$15,$15

	srl	t8,2,t8
	addl	$14,a4,a4
	xor	a3,ra,ra
	xor	$12,$15,$15

	srl	$15,31,t11
	addl	ra,a4,a4
	srl	a5,27,t10
	addl	$15,$15,$15

	or	t12,t8,t8
	addl	t10,a4,a4
	or	t11,$15,$15
	sll	a4,5,t11
	addl	AT,a3,a3
	zapnot	a4,0xf,a4
	ldl	$0,0(a0)

	sll	a5,30,t12
	addl	t11,a3,a3
	xor	a5,t8,ra
	ldl	$1,4(a0)

	srl	a5,2,a5
	addl	$15,a3,a3
	xor	t9,ra,ra
	ldl	$2,8(a0)

	srl	a4,27,t10
	addl	ra,a3,a3
	ldl	$3,12(a0)

	or	t12,a5,a5
	addl	t10,a3,a3
	ldl	$4,16(a0)
	addl	$0,a3,a3
	addl	$1,a4,a4
	addl	$2,a5,a5
	addl	$3,t8,t8
	addl	$4,t9,t9
	stl	a3,0(a0)
	stl	a4,4(a0)
	addq	a1,64,a1
	stl	a5,8(a0)
	stl	t8,12(a0)
	stl	t9,16(a0)
	cmpult	a1,a2,t11
	bne	t11,.Lloop

	.set	noreorder
	ldq	ra,0(sp)
	ldq	s0,8(sp)
	ldq	s1,16(sp)
	ldq	s2,24(sp)
	ldq	s3,32(sp)
	ldq	s4,40(sp)
	ldq	s5,48(sp)
	ldq	fp,56(sp)
	lda	sp,64(sp)
	ret	(ra)
.end	sha1_block_data_order
.ascii	"SHA1 block transform for Alpha, CRYPTOGAMS by <appro@openssl.org>"
.align	2
