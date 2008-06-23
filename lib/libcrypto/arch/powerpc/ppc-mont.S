.machine	"any"
.text

.globl	bn_mul_mont
.type	bn_mul_mont,@function
.align	4
bn_mul_mont:
	cmpwi	8,4
	mr	9,3
	li	3,0
	bclr	12,0

	slwi	8,8,2
	li	12,-4096
	addi	3,8,288
	subf	3,3,1
	and	3,3,12
	subf	3,1,3
	srwi	8,8,2
	stwux	1,1,3

	stw	14,16(1)
	stw	15,20(1)
	stw	16,24(1)
	stw	17,28(1)
	stw	18,32(1)
	stw	19,36(1)
	stw	20,40(1)
	stw	21,44(1)
	stw	22,48(1)
	stw	23,52(1)
	stw	24,56(1)
	stw	25,60(1)

	lwz	7,0(7)
	addi	8,8,-2

	lwz	17,0(5)
	lwz	10,0(4)
	addi	16,1,64
	mullw	19,10,17
	mulhwu	20,10,17

	lwz	10,4(4)
	lwz	11,0(6)

	mullw	18,19,7

	mullw	23,10,17
	mulhwu	24,10,17

	mullw	21,11,18
	mulhwu	22,11,18
	lwz	11,4(6)
	addc	21,21,19
	addze	22,22

	mullw	25,11,18
	mulhwu	0,11,18

	mtctr	8
	li	15,8
.align	4
.L1st:
	lwzx	10,4,15
	addc	19,23,20
	lwzx	11,6,15
	addze	20,24
	mullw	23,10,17
	addc	21,25,22
	mulhwu	24,10,17
	addze	22,0
	mullw	25,11,18
	addc	21,21,19
	mulhwu	0,11,18
	addze	22,22
	stw	21,0(16)

	addi	15,15,4
	addi	16,16,4
	bdnz+	.L1st

	addc	19,23,20
	addze	20,24

	addc	21,25,22
	addze	22,0
	addc	21,21,19
	addze	22,22
	stw	21,0(16)

	li	3,0
	addc	22,22,20
	addze	3,3
	stw	22,4(16)

	li	14,4
.align	4
.Louter:
	lwzx	17,5,14
	lwz	10,0(4)
	addi	16,1,64
	lwz	12,64(1)
	mullw	19,10,17
	mulhwu	20,10,17
	lwz	10,4(4)
	lwz	11,0(6)
	addc	19,19,12
	mullw	23,10,17
	addze	20,20
	mullw	18,19,7
	mulhwu	24,10,17
	mullw	21,11,18
	mulhwu	22,11,18
	lwz	11,4(6)
	addc	21,21,19
	mullw	25,11,18
	addze	22,22
	mulhwu	0,11,18

	mtctr	8
	li	15,8
.align	4
.Linner:
	lwzx	10,4,15
	addc	19,23,20
	lwz	12,4(16)
	addze	20,24
	lwzx	11,6,15
	addc	21,25,22
	mullw	23,10,17
	addze	22,0
	mulhwu	24,10,17
	addc	19,19,12
	mullw	25,11,18
	addze	20,20
	mulhwu	0,11,18
	addc	21,21,19
	addi	15,15,4
	addze	22,22
	stw	21,0(16)
	addi	16,16,4
	bdnz+	.Linner

	lwz	12,4(16)
	addc	19,23,20
	addze	20,24
	addc	19,19,12
	addze	20,20

	addc	21,25,22
	addze	22,0
	addc	21,21,19
	addze	22,22
	stw	21,0(16)

	addic	3,3,-1
	li	3,0
	adde	22,22,20
	addze	3,3
	stw	22,4(16)

	slwi	12,8,2
	.long	0x7c0e6040
	addi	14,14,4
	ble-	.Louter

	addi	8,8,2
	subfc	15,15,15
	addi	16,1,64
	mtctr	8

.align	4
.Lsub:	lwzx	12,16,15
	lwzx	11,6,15
	subfe	10,11,12
	stwx	10,9,15
	addi	15,15,4
	bdnz+	.Lsub

	li	15,0
	mtctr	8
	subfe	3,15,3
	and	4,16,3
	andc	6,9,3
	or	4,4,6

.align	4
.Lcopy:
	lwzx	12,4,15
	stwx	12,9,15
	stwx	15,16,15
	addi	15,15,4
	bdnz+	.Lcopy

	lwz	14,16(1)
	lwz	15,20(1)
	lwz	16,24(1)
	lwz	17,28(1)
	lwz	18,32(1)
	lwz	19,36(1)
	lwz	20,40(1)
	lwz	21,44(1)
	lwz	22,48(1)
	lwz	23,52(1)
	lwz	24,56(1)
	lwz	25,60(1)
	lwz	1,0(1)
	li	3,1
	blr	
.long	0
.byte	77,111,110,116,103,111,109,101,114,121,32,77,117,108,116,105,112,108,105,99,97,116,105,111,110,32,102,111,114,32,80,80,67,44,32,67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112,114,111,64,102,121,46,99,104,97,108,109,101,114,115,46,115,101,62,0
.align	2
