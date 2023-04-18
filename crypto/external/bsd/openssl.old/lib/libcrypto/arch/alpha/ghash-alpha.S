#ifdef __linux__
#include <asm/regdef.h>
#else
#include <asm.h>
#include <regdef.h>
#endif

.text

.set	noat
.set	noreorder
.globl	gcm_gmult_4bit
.align	4
.ent	gcm_gmult_4bit
gcm_gmult_4bit:
	.frame	sp,0,ra
	.prologue 0

	ldq	t11,8(a0)
	ldq	t10,0(a0)

	bsr	t0,picmeup
	nop
.align	4
	extbl	t11,7,a4
	and	a4,0xf0,a5
	sll	a4,4,a4
	and	a4,0xf0,a4

	addq	a4,a1,a4
	ldq	t9,8(a4)
	addq	a5,a1,a5
	ldq	t8,0(a4)

	and	t9,0x0f,t12
	sll	t8,60,t0
	lda	v0,6(zero)
	extbl	t11,6,a4

	ldq	t6,8(a5)
	s8addq	t12,AT,t12
	ldq	t5,0(a5)
	srl	t9,4,t9

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	and	a4,0xf0,a5

	xor	t6,t9,t9
	sll	a4,4,a4
	xor	t5,t8,t8
	and	a4,0xf0,a4

	addq	a4,a1,a4
	ldq	t4,8(a4)
	addq	a5,a1,a5
	ldq	t3,0(a4)

.Looplo1:
	and	t9,0x0f,t12
	sll	t8,60,t0
	subq	v0,1,v0
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	extbl	t11,v0,a4

	and	a4,0xf0,a5
	xor	t3,t8,t8
	xor	t4,t9,t9
	sll	a4,4,a4


	and	t9,0x0f,t12
	sll	t8,60,t0
	and	a4,0xf0,a4
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8
	addq	a4,a1,a4
	addq	a5,a1,a5

	ldq	t7,0(t12)
	srl	t8,4,t8
	ldq	t4,8(a4)
	xor	t0,t9,t9

	xor	t6,t9,t9
	xor	t5,t8,t8
	ldq	t3,0(a4)
	bne	v0,.Looplo1


	and	t9,0x0f,t12
	sll	t8,60,t0
	lda	v0,7(zero)
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	extbl	t10,v0,a4

	and	a4,0xf0,a5
	xor	t3,t8,t8
	xor	t4,t9,t9
	sll	a4,4,a4

	and	t9,0x0f,t12
	sll	t8,60,t0
	and	a4,0xf0,a4
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8
	addq	a4,a1,a4
	addq	a5,a1,a5

	ldq	t7,0(t12)
	srl	t8,4,t8
	ldq	t4,8(a4)
	xor	t0,t9,t9

	xor	t6,t9,t9
	xor	t5,t8,t8
	ldq	t3,0(a4)
	unop


.Loophi1:
	and	t9,0x0f,t12
	sll	t8,60,t0
	subq	v0,1,v0
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	extbl	t10,v0,a4

	and	a4,0xf0,a5
	xor	t3,t8,t8
	xor	t4,t9,t9
	sll	a4,4,a4


	and	t9,0x0f,t12
	sll	t8,60,t0
	and	a4,0xf0,a4
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8
	addq	a4,a1,a4
	addq	a5,a1,a5

	ldq	t7,0(t12)
	srl	t8,4,t8
	ldq	t4,8(a4)
	xor	t0,t9,t9

	xor	t6,t9,t9
	xor	t5,t8,t8
	ldq	t3,0(a4)
	bne	v0,.Loophi1


	and	t9,0x0f,t12
	sll	t8,60,t0
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9

	xor	t4,t9,t9
	xor	t3,t8,t8

	and	t9,0x0f,t12
	sll	t8,60,t0
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t6,t9,t9
	xor	t5,t8,t8
	xor	t0,t9,t9
	xor	t7,t8,t8
	srl	t9,24,t0	# byte swap
	srl	t9,8,t1

	sll	t9,8,t2
	sll	t9,24,t9
	zapnot	t0,0x11,t0
	zapnot	t1,0x22,t1

	zapnot	t9,0x88,t9
	or	t0,t1,t0
	zapnot	t2,0x44,t2

	or	t9,t0,t9
	srl	t8,24,t0
	srl	t8,8,t1

	or	t9,t2,t9
	sll	t8,8,t2
	sll	t8,24,t8

	srl	t9,32,t11
	sll	t9,32,t9

	zapnot	t0,0x11,t0
	zapnot	t1,0x22,t1
	or	t9,t11,t11

	zapnot	t8,0x88,t8
	or	t0,t1,t0
	zapnot	t2,0x44,t2

	or	t8,t0,t8
	or	t8,t2,t8

	srl	t8,32,t10
	sll	t8,32,t8

	or	t8,t10,t10
	stq	t11,8(a0)
	stq	t10,0(a0)

	ret	(ra)
.end	gcm_gmult_4bit
.globl	gcm_ghash_4bit
.align	4
.ent	gcm_ghash_4bit
gcm_ghash_4bit:
	lda	sp,-32(sp)
	stq	ra,0(sp)
	stq	s0,8(sp)
	stq	s1,16(sp)
	.mask	0x04000600,-32
	.frame	sp,32,ra
	.prologue 0

	ldq_u	s0,0(a2)
	ldq_u	t3,7(a2)
	ldq_u	s1,8(a2)
	ldq_u	t4,15(a2)
	ldq	t10,0(a0)
	ldq	t11,8(a0)

	bsr	t0,picmeup
	nop

.Louter:
	extql	s0,a2,s0
	extqh	t3,a2,t3
	or	s0,t3,s0
	lda	a2,16(a2)

	extql	s1,a2,s1
	extqh	t4,a2,t4
	or	s1,t4,s1
	subq	a3,16,a3

	xor	t11,s1,t11
	xor	t10,s0,t10
.align	4
	extbl	t11,7,a4
	and	a4,0xf0,a5
	sll	a4,4,a4
	and	a4,0xf0,a4

	addq	a4,a1,a4
	ldq	t9,8(a4)
	addq	a5,a1,a5
	ldq	t8,0(a4)

	and	t9,0x0f,t12
	sll	t8,60,t0
	lda	v0,6(zero)
	extbl	t11,6,a4

	ldq	t6,8(a5)
	s8addq	t12,AT,t12
	ldq	t5,0(a5)
	srl	t9,4,t9

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	and	a4,0xf0,a5

	xor	t6,t9,t9
	sll	a4,4,a4
	xor	t5,t8,t8
	and	a4,0xf0,a4

	addq	a4,a1,a4
	ldq	t4,8(a4)
	addq	a5,a1,a5
	ldq	t3,0(a4)

.Looplo2:
	and	t9,0x0f,t12
	sll	t8,60,t0
	subq	v0,1,v0
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	extbl	t11,v0,a4

	and	a4,0xf0,a5
	xor	t3,t8,t8
	xor	t4,t9,t9
	sll	a4,4,a4


	and	t9,0x0f,t12
	sll	t8,60,t0
	and	a4,0xf0,a4
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8
	addq	a4,a1,a4
	addq	a5,a1,a5

	ldq	t7,0(t12)
	srl	t8,4,t8
	ldq	t4,8(a4)
	xor	t0,t9,t9

	xor	t6,t9,t9
	xor	t5,t8,t8
	ldq	t3,0(a4)
	bne	v0,.Looplo2


	and	t9,0x0f,t12
	sll	t8,60,t0
	lda	v0,7(zero)
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	extbl	t10,v0,a4

	and	a4,0xf0,a5
	xor	t3,t8,t8
	xor	t4,t9,t9
	sll	a4,4,a4

	and	t9,0x0f,t12
	sll	t8,60,t0
	and	a4,0xf0,a4
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8
	addq	a4,a1,a4
	addq	a5,a1,a5

	ldq	t7,0(t12)
	srl	t8,4,t8
	ldq	t4,8(a4)
	xor	t0,t9,t9

	xor	t6,t9,t9
	xor	t5,t8,t8
	ldq	t3,0(a4)
	unop


.Loophi2:
	and	t9,0x0f,t12
	sll	t8,60,t0
	subq	v0,1,v0
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9
	extbl	t10,v0,a4

	and	a4,0xf0,a5
	xor	t3,t8,t8
	xor	t4,t9,t9
	sll	a4,4,a4


	and	t9,0x0f,t12
	sll	t8,60,t0
	and	a4,0xf0,a4
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8
	addq	a4,a1,a4
	addq	a5,a1,a5

	ldq	t7,0(t12)
	srl	t8,4,t8
	ldq	t4,8(a4)
	xor	t0,t9,t9

	xor	t6,t9,t9
	xor	t5,t8,t8
	ldq	t3,0(a4)
	bne	v0,.Loophi2


	and	t9,0x0f,t12
	sll	t8,60,t0
	srl	t9,4,t9

	ldq	t6,8(a5)
	xor	t7,t8,t8
	ldq	t5,0(a5)
	s8addq	t12,AT,t12

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t0,t9,t9

	xor	t4,t9,t9
	xor	t3,t8,t8

	and	t9,0x0f,t12
	sll	t8,60,t0
	srl	t9,4,t9

	s8addq	t12,AT,t12
	xor	t7,t8,t8

	ldq	t7,0(t12)
	srl	t8,4,t8
	xor	t6,t9,t9
	xor	t5,t8,t8
	xor	t0,t9,t9
	xor	t7,t8,t8
	srl	t9,24,t0	# byte swap
	srl	t9,8,t1

	sll	t9,8,t2
	sll	t9,24,t9
	zapnot	t0,0x11,t0
	zapnot	t1,0x22,t1

	zapnot	t9,0x88,t9
	or	t0,t1,t0
	zapnot	t2,0x44,t2

	or	t9,t0,t9
	srl	t8,24,t0
	srl	t8,8,t1

	or	t9,t2,t9
	sll	t8,8,t2
	sll	t8,24,t8

	srl	t9,32,t11
	sll	t9,32,t9
	beq	a3,.Ldone

	zapnot	t0,0x11,t0
	zapnot	t1,0x22,t1
	or	t9,t11,t11
	ldq_u	s0,0(a2)

	zapnot	t8,0x88,t8
	or	t0,t1,t0
	zapnot	t2,0x44,t2
	ldq_u	t3,7(a2)

	or	t8,t0,t8
	or	t8,t2,t8
	ldq_u	s1,8(a2)
	ldq_u	t4,15(a2)

	srl	t8,32,t10
	sll	t8,32,t8

	or	t8,t10,t10
	br	zero,.Louter

.Ldone:
	zapnot	t0,0x11,t0
	zapnot	t1,0x22,t1
	or	t9,t11,t11

	zapnot	t8,0x88,t8
	or	t0,t1,t0
	zapnot	t2,0x44,t2

	or	t8,t0,t8
	or	t8,t2,t8

	srl	t8,32,t10
	sll	t8,32,t8

	or	t8,t10,t10

	stq	t11,8(a0)
	stq	t10,0(a0)

	.set	noreorder
	/*ldq	ra,0(sp)*/
	ldq	s0,8(sp)
	ldq	s1,16(sp)
	lda	sp,32(sp)
	ret	(ra)
.end	gcm_ghash_4bit

.align	4
.ent	picmeup
picmeup:
	.frame	sp,0,t0
	.prologue 0
	br	AT,.Lpic
.Lpic:	lda	AT,12(AT)
	ret	(t0)
.end	picmeup
	nop
rem_4bit:
	.long	0,0x0000<<16, 0,0x1C20<<16, 0,0x3840<<16, 0,0x2460<<16
	.long	0,0x7080<<16, 0,0x6CA0<<16, 0,0x48C0<<16, 0,0x54E0<<16
	.long	0,0xE100<<16, 0,0xFD20<<16, 0,0xD940<<16, 0,0xC560<<16
	.long	0,0x9180<<16, 0,0x8DA0<<16, 0,0xA9C0<<16, 0,0xB5E0<<16
.ascii	"GHASH for Alpha, CRYPTOGAMS by <appro@openssl.org>"
.align	4

