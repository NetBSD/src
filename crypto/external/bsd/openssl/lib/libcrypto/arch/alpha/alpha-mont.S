#ifdef __linux__
#include <asm/regdef.h>
#else
#include <asm.h>
#include <regdef.h>
#endif

.text

.set	noat
.set	noreorder

.globl	bn_mul_mont
.align	5
.ent	bn_mul_mont
bn_mul_mont:
	lda	sp,-48(sp)
	stq	ra,0(sp)
	stq	s3,8(sp)
	stq	s4,16(sp)
	stq	s5,24(sp)
	stq	fp,32(sp)
	mov	sp,fp
	.mask	0x0400f000,-48
	.frame	fp,48,ra
	.prologue 0

	.align	4
	.set	reorder
	sextl	a5,a5
	mov	0,v0
	cmplt	a5,4,AT
	bne	AT,.Lexit

	ldq	t1,0(a1)	# ap[0]
	s8addq	a5,16,AT
	ldq	t4,8(a1)
	subq	sp,AT,sp
	ldq	t5,0(a2)	# bp[0]
	lda	AT,-4096(zero)	# mov	-4096,AT
	ldq	a4,0(a4)
	and	sp,AT,sp

	mulq	t1,t5,t0
	ldq	t3,0(a3)	# np[0]
	umulh	t1,t5,t1
	ldq	t6,8(a3)

	mulq	t0,a4,s5

	mulq	t3,s5,t2
	umulh	t3,s5,t3

	addq	t2,t0,t2
	cmpult	t2,t0,AT
	addq	t3,AT,t3

	mulq	t4,t5,t8
	mov	2,s4
	umulh	t4,t5,t9
	mov	sp,t7

	mulq	t6,s5,t10
	s8addq	s4,a1,t4
	umulh	t6,s5,t11
	s8addq	s4,a3,t6
.align	4
.L1st:
	.set	noreorder
	ldq	t4,0(t4)
	addl	s4,1,s4
	ldq	t6,0(t6)
	lda	t7,8(t7)

	addq	t8,t1,t0
	mulq	t4,t5,t8
	cmpult	t0,t1,AT
	addq	t10,t3,t2

	mulq	t6,s5,t10
	addq	t9,AT,t1
	cmpult	t2,t3,v0
	cmplt	s4,a5,t12

	umulh	t4,t5,t9
	addq	t11,v0,t3
	addq	t2,t0,t2
	s8addq	s4,a1,t4

	umulh	t6,s5,t11
	cmpult	t2,t0,v0
	addq	t3,v0,t3
	s8addq	s4,a3,t6

	stq	t2,-8(t7)
	nop
	unop
	bne	t12,.L1st
	.set	reorder

	addq	t8,t1,t0
	addq	t10,t3,t2
	cmpult	t0,t1,AT
	cmpult	t2,t3,v0
	addq	t9,AT,t1
	addq	t11,v0,t3

	addq	t2,t0,t2
	cmpult	t2,t0,v0
	addq	t3,v0,t3

	stq	t2,0(t7)

	addq	t3,t1,t3
	cmpult	t3,t1,AT
	stq	t3,8(t7)
	stq	AT,16(t7)

	mov	1,s3
.align	4
.Louter:
	s8addq	s3,a2,t5
	ldq	t1,0(a1)
	ldq	t4,8(a1)
	ldq	t5,0(t5)
	ldq	t3,0(a3)
	ldq	t6,8(a3)
	ldq	t12,0(sp)

	mulq	t1,t5,t0
	umulh	t1,t5,t1

	addq	t0,t12,t0
	cmpult	t0,t12,AT
	addq	t1,AT,t1

	mulq	t0,a4,s5

	mulq	t3,s5,t2
	umulh	t3,s5,t3

	addq	t2,t0,t2
	cmpult	t2,t0,AT
	mov	2,s4
	addq	t3,AT,t3

	mulq	t4,t5,t8
	mov	sp,t7
	umulh	t4,t5,t9

	mulq	t6,s5,t10
	s8addq	s4,a1,t4
	umulh	t6,s5,t11
.align	4
.Linner:
	.set	noreorder
	ldq	t12,8(t7)	#L0
	nop			#U1
	ldq	t4,0(t4)	#L1
	s8addq	s4,a3,t6	#U0

	ldq	t6,0(t6)	#L0
	nop			#U1
	addq	t8,t1,t0	#L1
	lda	t7,8(t7)

	mulq	t4,t5,t8	#U1
	cmpult	t0,t1,AT	#L0
	addq	t10,t3,t2	#L1
	addl	s4,1,s4

	mulq	t6,s5,t10	#U1
	addq	t9,AT,t1	#L0
	addq	t0,t12,t0	#L1
	cmpult	t2,t3,v0	#U0

	umulh	t4,t5,t9	#U1
	cmpult	t0,t12,AT	#L0
	addq	t2,t0,t2	#L1
	addq	t11,v0,t3	#U0

	umulh	t6,s5,t11	#U1
	s8addq	s4,a1,t4	#L0
	cmpult	t2,t0,v0	#L1
	cmplt	s4,a5,t12	#U0	# borrow t12

	addq	t1,AT,t1	#L0
	addq	t3,v0,t3	#U1
	stq	t2,-8(t7)	#L1
	bne	t12,.Linner	#U0
	.set	reorder

	ldq	t12,8(t7)
	addq	t8,t1,t0
	addq	t10,t3,t2
	cmpult	t0,t1,AT
	cmpult	t2,t3,v0
	addq	t9,AT,t1
	addq	t11,v0,t3

	addq	t0,t12,t0
	cmpult	t0,t12,AT
	addq	t1,AT,t1

	ldq	t12,16(t7)
	addq	t2,t0,s4
	cmpult	s4,t0,v0
	addq	t3,v0,t3

	addq	t3,t1,t2
	stq	s4,0(t7)
	cmpult	t2,t1,t3
	addq	t2,t12,t2
	cmpult	t2,t12,AT
	addl	s3,1,s3
	addq	t3,AT,t3
	stq	t2,8(t7)
	cmplt	s3,a5,t12	# borrow t12
	stq	t3,16(t7)
	bne	t12,.Louter

	s8addq	a5,sp,t12	# &tp[num]
	mov	a0,a2		# put rp aside
	mov	sp,t7
	mov	sp,a1
	mov	0,t1		# clear borrow bit

.align	4
.Lsub:	ldq	t0,0(t7)
	ldq	t2,0(a3)
	lda	t7,8(t7)
	lda	a3,8(a3)
	subq	t0,t2,t2	# tp[i]-np[i]
	cmpult	t0,t2,AT
	subq	t2,t1,t0
	cmpult	t2,t0,t1
	or	t1,AT,t1
	stq	t0,0(a0)
	cmpult	t7,t12,v0
	lda	a0,8(a0)
	bne	v0,.Lsub

	subq	t3,t1,t1	# handle upmost overflow bit
	mov	sp,t7
	mov	a2,a0		# restore rp

	and	sp,t1,a1
	bic	a2,t1,a2
	bis	a2,a1,a1	# ap=borrow?tp:rp

.align	4
.Lcopy:	ldq	t4,0(a1)	# copy or in-place refresh
	lda	t7,8(t7)
	lda	a0,8(a0)
	lda	a1,8(a1)
	stq	zero,-8(t7)	# zap tp
	cmpult	t7,t12,AT
	stq	t4,-8(a0)
	bne	AT,.Lcopy
	mov	1,v0

.Lexit:
	.set	noreorder
	mov	fp,sp
	/*ldq	ra,0(sp)*/
	ldq	s3,8(sp)
	ldq	s4,16(sp)
	ldq	s5,24(sp)
	ldq	fp,32(sp)
	lda	sp,48(sp)
	ret	(ra)
.end	bn_mul_mont
.ascii	"Montgomery Multiplication for Alpha, CRYPTOGAMS by <appro@openssl.org>"
.align	2
