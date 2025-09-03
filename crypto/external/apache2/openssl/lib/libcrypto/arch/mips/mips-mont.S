#include "mips_arch.h"

.text

.set	noat
.set	noreorder

.align	5
.globl	bn_mul_mont
.ent	bn_mul_mont
bn_mul_mont:
	lw	$8,16($29)
	lw	$9,20($29)
	slt	$1,$9,4
	bnez	$1,1f
	li	$2,0
	slt	$1,$9,17	# on in-order CPU
	bnez	$1,bn_mul_mont_internal
	nop
1:	jr	$31
	li	$4,0
.end	bn_mul_mont

.align	5
.ent	bn_mul_mont_internal
bn_mul_mont_internal:
	.frame	$30,14*4,$31
	.mask	0x40000000|16711680,-4
	subu $29,14*4
	sw	$30,(14-1)*4($29)
	sw	$23,(14-2)*4($29)
	sw	$22,(14-3)*4($29)
	sw	$21,(14-4)*4($29)
	sw	$20,(14-5)*4($29)
	sw	$19,(14-6)*4($29)
	sw	$18,(14-7)*4($29)
	sw	$17,(14-8)*4($29)
	sw	$16,(14-9)*4($29)
	move	$30,$29

	.set	reorder
	lw	$8,0($8)
	lw	$13,0($6)	# bp[0]
	lw	$12,0($5)	# ap[0]
	lw	$14,0($7)	# np[0]

	subu $29,2*4	# place for two extra words
	sll	$9,2
	li	$1,-4096
	subu $29,$9
	and	$29,$1

	multu	($12,$13)
	lw	$17,4($5)
	lw	$19,4($7)
	mflo	($10,$12,$13)
	mfhi	($11,$12,$13)
	multu	($10,$8)
	mflo	($23,$10,$8)

	multu	($17,$13)
	mflo	($16,$17,$13)
	mfhi	($17,$17,$13)

	multu	($14,$23)
	mflo	($24,$14,$23)
	mfhi	($25,$14,$23)
	multu	($19,$23)
	addu	$24,$10
	sltu	$1,$24,$10
	addu	$25,$1
	mflo	($18,$19,$23)
	mfhi	($19,$19,$23)

	move	$15,$29
	li	$22,2*4
.align	4
.L1st:
	.set	noreorder
	addu $12,$5,$22
	addu $14,$7,$22
	lw	$12,($12)
	lw	$14,($14)

	multu	($12,$13)
	addu	$10,$16,$11
	addu	$24,$18,$25
	sltu	$1,$10,$11
	sltu	$2,$24,$25
	addu	$11,$17,$1
	addu	$25,$19,$2
	mflo	($16,$12,$13)
	mfhi	($17,$12,$13)

	addu	$24,$10
	sltu	$1,$24,$10
	multu	($14,$23)
	addu	$25,$1
	addu	$22,4
	sw	$24,($15)
	sltu	$2,$22,$9
	mflo	($18,$14,$23)
	mfhi	($19,$14,$23)

	bnez	$2,.L1st
	addu $15,4
	.set	reorder

	addu	$10,$16,$11
	sltu	$1,$10,$11
	addu	$11,$17,$1

	addu	$24,$18,$25
	sltu	$2,$24,$25
	addu	$25,$19,$2
	addu	$24,$10
	sltu	$1,$24,$10
	addu	$25,$1

	sw	$24,($15)

	addu	$25,$11
	sltu	$1,$25,$11
	sw	$25,4($15)
	sw	$1,2*4($15)

	li	$21,4
.align	4
.Louter:
	addu $13,$6,$21
	lw	$13,($13)
	lw	$12,($5)
	lw	$17,4($5)
	lw	$20,($29)

	multu	($12,$13)
	lw	$14,($7)
	lw	$19,4($7)
	mflo	($10,$12,$13)
	mfhi	($11,$12,$13)
	addu	$10,$20
	multu	($10,$8)
	sltu	$1,$10,$20
	addu	$11,$1
	mflo	($23,$10,$8)

	multu	($17,$13)
	mflo	($16,$17,$13)
	mfhi	($17,$17,$13)

	multu	($14,$23)
	mflo	($24,$14,$23)
	mfhi	($25,$14,$23)

	multu	($19,$23)
	addu	$24,$10
	sltu	$1,$24,$10
	addu	$25,$1
	mflo	($18,$19,$23)
	mfhi	($19,$19,$23)

	move	$15,$29
	li	$22,2*4
	lw	$20,4($15)
.align	4
.Linner:
	.set	noreorder
	addu $12,$5,$22
	addu $14,$7,$22
	lw	$12,($12)
	lw	$14,($14)

	multu	($12,$13)
	addu	$10,$16,$11
	addu	$24,$18,$25
	sltu	$1,$10,$11
	sltu	$2,$24,$25
	addu	$11,$17,$1
	addu	$25,$19,$2
	mflo	($16,$12,$13)
	mfhi	($17,$12,$13)

	addu	$10,$20
	addu	$22,4
	multu	($14,$23)
	sltu	$1,$10,$20
	addu	$24,$10
	addu	$11,$1
	sltu	$2,$24,$10
	lw	$20,2*4($15)
	addu	$25,$2
	sltu	$1,$22,$9
	mflo	($18,$14,$23)
	mfhi	($19,$14,$23)
	sw	$24,($15)
	bnez	$1,.Linner
	addu $15,4
	.set	reorder

	addu	$10,$16,$11
	sltu	$1,$10,$11
	addu	$11,$17,$1
	addu	$10,$20
	sltu	$2,$10,$20
	addu	$11,$2

	lw	$20,2*4($15)
	addu	$24,$18,$25
	sltu	$1,$24,$25
	addu	$25,$19,$1
	addu	$24,$10
	sltu	$2,$24,$10
	addu	$25,$2
	sw	$24,($15)

	addu	$24,$25,$11
	sltu	$25,$24,$11
	addu	$24,$20
	sltu	$1,$24,$20
	addu	$25,$1
	sw	$24,4($15)
	sw	$25,2*4($15)

	addu	$21,4
	sltu	$2,$21,$9
	bnez	$2,.Louter

	.set	noreorder
	addu $20,$29,$9	# &tp[num]
	move	$15,$29
	move	$5,$29
	li	$11,0		# clear borrow bit

.align	4
.Lsub:	lw	$10,($15)
	lw	$24,($7)
	addu $15,4
	addu $7,4
	subu	$24,$10,$24	# tp[i]-np[i]
	sgtu	$1,$24,$10
	subu	$10,$24,$11
	sgtu	$11,$10,$24
	sw	$10,($4)
	or	$11,$1
	sltu	$1,$15,$20
	bnez	$1,.Lsub
	addu $4,4

	subu	$11,$25,$11	# handle upmost overflow bit
	move	$15,$29
	subu $4,$9	# restore rp
	not	$25,$11

.Lcopy:	lw	$14,($15)	# conditional move
	lw	$12,($4)
	sw	$0,($15)
	addu $15,4
	and	$14,$11
	and	$12,$25
	or	$12,$14
	sltu	$1,$15,$20
	sw	$12,($4)
	bnez	$1,.Lcopy
	addu $4,4

	li	$4,1
	li	$2,1

	.set	noreorder
	move	$29,$30
	lw	$30,(14-1)*4($29)
	lw	$23,(14-2)*4($29)
	lw	$22,(14-3)*4($29)
	lw	$21,(14-4)*4($29)
	lw	$20,(14-5)*4($29)
	lw	$19,(14-6)*4($29)
	lw	$18,(14-7)*4($29)
	lw	$17,(14-8)*4($29)
	lw	$16,(14-9)*4($29)
	jr	$31
	addu $29,14*4
.end	bn_mul_mont_internal
.rdata
.asciiz	"Montgomery Multiplication for MIPS, CRYPTOGAMS by <appro@openssl.org>"
