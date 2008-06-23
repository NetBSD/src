.text

.globl	OPENSSL_cpuid_setup
.type	OPENSSL_cpuid_setup,@function
.align	4
OPENSSL_cpuid_setup:
	blr	

.globl	OPENSSL_wipe_cpu
.type	OPENSSL_wipe_cpu,@function
.align	4
OPENSSL_wipe_cpu:
	xor	0,0,0
	mr	3,1
	xor	4,4,4
	xor	5,5,5
	xor	6,6,6
	xor	7,7,7
	xor	8,8,8
	xor	9,9,9
	xor	10,10,10
	xor	11,11,11
	xor	12,12,12
	blr	

.globl	OPENSSL_atomic_add
.type	OPENSSL_atomic_add,@function
.align	4
OPENSSL_atomic_add:
.Loop:	lwarx	5,0,3
	add	0,4,5
	stwcx.	0,0,3
	bne-	.Loop
	mr	3,0
	blr	

.globl	OPENSSL_rdtsc
.type	OPENSSL_rdtsc,@function
.align	4
OPENSSL_rdtsc:
	mftb	3
	mftbu	4
	blr	

.globl	OPENSSL_cleanse
.type	OPENSSL_cleanse,@function
.align	4
OPENSSL_cleanse:
	cmplwi	4,7
	li	0,0
	bge	.Lot
.Little:	mtctr	4
	stb	0,0(3)
	addi	3,3,1
	bdnz+	$-8
	blr	
.Lot:	andi.	5,3,3
	beq	.Laligned
	stb	0,0(3)
	subi	4,4,1
	addi	3,3,1
	b	.Lot
.Laligned:
	srwi	5,4,2
	mtctr	5
	stw	0,0(3)
	addi	3,3,4
	bdnz+	$-8
	andi.	4,4,3
	bne	.Little
	blr	
