.machine	"any"
.text

.macro	SAVE_REGS
	mflr	0
	std	0, 16(1)
	stdu	1,-512(1)

	std	14, 112(1)
	std	15, 120(1)
	std	16, 128(1)
	std	17, 136(1)
	std	18, 144(1)
	std	19, 152(1)
	std	20, 160(1)
	std	21, 168(1)
	std	22, 176(1)
	std	23, 184(1)
	std	24, 192(1)

	stxv	32+20, 256(1)
	stxv	32+21, 256+16(1)
	stxv	32+22, 256+32(1)
	stxv	32+23, 256+48(1)
	stxv	32+24, 256+64(1)
	stxv	32+25, 256+80(1)
	stxv	32+26, 256+96(1)
	stxv	32+27, 256+112(1)
	stxv	32+28, 256+128(1)
	stxv	32+29, 256+144(1)
	stxv	32+30, 256+160(1)
	stxv	32+31, 256+176(1)
.endm	

.macro	RESTORE_REGS
	lxv	32+20, 256(1)
	lxv	32+21, 256+16(1)
	lxv	32+22, 256+32(1)
	lxv	32+23, 256+48(1)
	lxv	32+24, 256+64(1)
	lxv	32+25, 256+80(1)
	lxv	32+26, 256+96(1)
	lxv	32+27, 256+112(1)
	lxv	32+28, 256+128(1)
	lxv	32+29, 256+144(1)
	lxv	32+30, 256+160(1)
	lxv	32+31, 256+176(1)

	ld	14, 112(1)
	ld	15, 120(1)
	ld	16, 128(1)
	ld	17, 136(1)
	ld	18, 144(1)
	ld	19, 152(1)
	ld	20, 160(1)
	ld	21, 168(1)
	ld	22, 176(1)
	ld	23, 184(1)
	ld	24, 192(1)

	addi	1, 1, 512
	ld	0, 16(1)
	mtlr	0
.endm	


.macro	AES_CIPHER_4x r
	.long	0x11EF0508
	.long	0x12100508
	.long	0x12310508
	.long	0x12520508
.endm	


.macro	AES_CIPHER_8x r
	.long	0x11EF0508
	.long	0x12100508
	.long	0x12310508
	.long	0x12520508
	.long	0x12730508
	.long	0x12940508
	.long	0x12B50508
	.long	0x12D60508
.endm	

.macro	.LOOP_8AES_STATE
	AES_CIPHER_8x	23
	AES_CIPHER_8x	24
	AES_CIPHER_8x	25
	AES_CIPHER_8x	26
	AES_CIPHER_8x	27
	AES_CIPHER_8x	28
	AES_CIPHER_8x	29
	AES_CIPHER_8x	1
.endm	












.macro	PPC_GFMUL128_8x

	.long	0x12EC7CC8
	.long	0x130984C8
	.long	0x13268CC8
	.long	0x134394C8

	vxor	23, 23, 24
	vxor	23, 23, 25
	vxor	23, 23, 26

	.long	0x136D7CC8
	.long	0x138A84C8
	.long	0x13278CC8
	.long	0x134494C8

	vxor	24, 27, 28
	vxor	24, 24, 25
	vxor	24, 24, 26

	.long	0x134E7CC8
	.long	0x136B84C8
	.long	0x13888CC8
	.long	0x13A594C8

	vxor	26, 26, 27
	vxor	26, 26, 28
	vxor	26, 26, 29


	.long	0x139714C8

	vxor	1, 1, 1
	vsldoi	25, 24, 1, 8
	vsldoi	1, 1, 24, 8
	vxor	23, 23, 25




	xxlor	32+29, 10, 10
	vpermxor	23, 23, 28, 29

	vxor	24, 26, 1







	vpermxor	27, 23, 24, 29
	.long	0x12F714C8
	vxor	0, 23, 27

	vxor	19, 19, 0


	.long	0x1309A4C8
	.long	0x1326ACC8
	.long	0x1343B4C8
	.long	0x12EC9CC8

	vxor	23, 23, 24
	vxor	23, 23, 25
	vxor	23, 23, 26

	.long	0x136D9CC8
	.long	0x138AA4C8
	.long	0x1327ACC8
	.long	0x1344B4C8

	vxor	24, 27, 28
	vxor	24, 24, 25
	vxor	24, 24, 26

	.long	0x134E9CC8
	.long	0x136BA4C8
	.long	0x1388ACC8
	.long	0x13A5B4C8

	vxor	26, 26, 27
	vxor	26, 26, 28
	vxor	26, 26, 29


	.long	0x139714C8

	vxor	1, 1, 1
	vsldoi	25, 24, 1, 8
	vsldoi	1, 1, 24, 8
	vxor	23, 23, 25




	xxlor	32+29, 10, 10
	vpermxor	23, 23, 28, 29

	vxor	24, 26, 1







	vpermxor	27, 23, 24, 29
	.long	0x12F714C8
	vxor	0, 23, 27
.endm	






.macro	PPC_GHASH1x H S1

	vxor	1, 1, 1

	.long	0x12C304C8
	.long	0x12E404C8
	.long	0x130504C8

	.long	0x137614C8

	vsldoi	25, 23, 1, 8
	vsldoi	26, 1, 23, 8
	vxor	22, 22, 25
	vxor	24, 24, 26

	xxlor	32+25, 10, 10
	vpermxor	22, 22, 27, 25




	vpermxor	23, 22, 24, 25
	.long	0x12D614C8

	vxor	\H, 22, 23
.endm	







.macro	.LOAD_HASH_TABLE

	lxvb16x	32, 0, 8

	vxor	1, 1, 1

	li	10, 32
	lxvd2x	2+32, 10, 8


	li	10, 64
	lxvd2x	4+32, 10, 8
	vsldoi	3, 1, 4, 8
	vsldoi	5, 4, 1, 8
	li	10, 112
	lxvd2x	7+32, 10, 8
	vsldoi	6, 1, 7, 8
	vsldoi	8, 7, 1, 8
	li	10, 160
	lxvd2x	10+32, 10, 8
	vsldoi	9, 1, 10, 8
	vsldoi	11, 10, 1, 8
	li	10, 208
	lxvd2x	13+32, 10, 8
	vsldoi	12, 1, 13, 8
	vsldoi	14, 13, 1, 8
.endm	

.macro	PROCESS_8X_AES_STATES
	.long	0x11EF0D09
	.long	0x12100D09
	.long	0x12310D09
	.long	0x12520D09
	.long	0x12730D09
	.long	0x12940D09
	.long	0x12B50D09
	.long	0x12D60D09

	lxvb16x	32+23, 0, 14
	lxvb16x	32+24, 15, 14
	lxvb16x	32+25, 16, 14
	lxvb16x	32+26, 17, 14
	lxvb16x	32+27, 18, 14
	lxvb16x	32+28, 19, 14
	lxvb16x	32+29, 20, 14
	lxvb16x	32+30, 21, 14
	addi	14, 14, 128

	vxor	15, 15, 23
	vxor	16, 16, 24
	vxor	17, 17, 25
	vxor	18, 18, 26
	vxor	19, 19, 27
	vxor	20, 20, 28
	vxor	21, 21, 29
	vxor	22, 22, 30

	stxvb16x	47, 0, 9
	stxvb16x	48, 15, 9
	stxvb16x	49, 16, 9
	stxvb16x	50, 17, 9
	stxvb16x	51, 18, 9
	stxvb16x	52, 19, 9
	stxvb16x	53, 20, 9
	stxvb16x	54, 21, 9
	addi	9, 9, 128
.endm	

.macro	COMPUTE_STATES
	xxlor	32+15, 9, 9
	vadduwm	15, 15, 31
	vadduwm	16, 15, 31
	vadduwm	17, 16, 31
	vadduwm	18, 17, 31
	vadduwm	19, 18, 31
	vadduwm	20, 19, 31
	vadduwm	21, 20, 31
	vadduwm	22, 21, 31
	xxlor	9, 32+22, 32+22

	xxlxor	32+15, 32+15, 0
	xxlxor	32+16, 32+16, 0
	xxlxor	32+17, 32+17, 0
	xxlxor	32+18, 32+18, 0
	xxlxor	32+19, 32+19, 0
	xxlxor	32+20, 32+20, 0
	xxlxor	32+21, 32+21, 0
	xxlxor	32+22, 32+22, 0
.endm	








.align	4
aes_gcm_crypt_1x:
.localentry	aes_gcm_crypt_1x,0

	cmpdi	5, 16
	bge	__More_1x
	blr	
__More_1x:
	li	10, 16
	divdu	12, 5, 10

	xxlxor	32+15, 32+30, 0


	lxv	32+16, 16(6)
	lxv	32+17, 32(6)
	lxv	32+18, 48(6)
	lxv	32+19, 64(6)
	lxv	32+20, 80(6)
	lxv	32+21, 96(6)
	lxv	32+28, 112(6)
	lxv	32+29, 128(6)

	lwz	23, 240(6)
	addi	22, 23, -9

	cmpdi	12, 0
	bgt	__Loop_1x
	blr	

__Loop_1x:
	mtctr	22
	addi	10, 6, 144
	.long	0x11EF8508
	.long	0x11EF8D08
	.long	0x11EF9508
	.long	0x11EF9D08
	.long	0x11EFA508
	.long	0x11EFAD08
	.long	0x11EFE508
	.long	0x11EFED08

__Loop_aes_1state:
	lxv	32+1, 0(10)
	.long	0x11EF0D08
	addi	10, 10, 16
	bdnz	__Loop_aes_1state
	lxv	32+1, 0(10)
	lxvb16x	11, 0, 14
	.long	0x11EF0D09

	xxlxor	32+15, 32+15, 11
	stxvb16x	32+15, 0, 9
	addi	14, 14, 16
	addi	9, 9, 16

	cmpdi	24, 0
	bne	__Encrypt_1x
	xxlor	15+32, 11, 11
__Encrypt_1x:
	vxor	15, 15, 0
	PPC_GHASH1x	0, 15

	addi	5, 5, -16
	addi	11, 11, 16

	vadduwm	30, 30, 31
	xxlxor	32+15, 32+30, 0
	addi	12, 12, -1
	cmpdi	12, 0
	bgt	__Loop_1x

	stxvb16x	32+0, 0, 8
	blr	
.size	aes_gcm_crypt_1x,.-aes_gcm_crypt_1x








.align	4
__Process_partial:
.localentry	__Process_partial,0


	vspltisb	16,-1
	li	12, 16
	sub	12, 12, 5
	sldi	12, 12, 3
	mtvsrdd	32+17, 0, 12
	vslo	16, 16, 17

	lxvb16x	11, 0, 14
	xxland	11, 11, 32+16


	xxlxor	32+15, 32+30, 0
	lwz	23, 240(6)
	addi	22, 23, -1
	mtctr	22
	addi	10, 6, 16

__Loop_aes_pstate:
	lxv	32+1, 0(10)
	.long	0x11EF0D08
	addi	10, 10, 16
	bdnz	__Loop_aes_pstate
	lxv	32+1, 0(10)
	.long	0x11EF0D09

	xxlxor	32+15, 32+15, 11
	vand	15, 15, 16



	li	10, 224
	stxvb16x	15+32, 10, 1
	addi	10, 1, 223
	addi	12, 9, -1
	mtctr	5
__Write_partial:
	lbzu	22, 1(10)
	stbu	22, 1(12)
	bdnz	__Write_partial

	cmpdi	24, 0
	bne	__Encrypt_partial
	xxlor	32+15, 11, 11
__Encrypt_partial:
	vxor	15, 15, 0
	PPC_GHASH1x	0, 15
	li	5, 0
	stxvb16x	32+0, 0, 8
	blr	
.size	__Process_partial,.-__Process_partial
















.global	ppc_aes_gcm_encrypt
.align	5
ppc_aes_gcm_encrypt:
.localentry	ppc_aes_gcm_encrypt,0

	SAVE_REGS	
.LOAD_HASH_TABLE	


	lxvb16x	30+32, 0, 7

	mr	14, 3
	mr	9, 4


	vxor	31, 31, 31
	vspltisb	22,1
	vsldoi	31, 31, 22,1

	addis	11, 2, permx@toc@ha
	addi	11, 11, permx@toc@l
	lxv	10, 0(11)
	li	11, 0

	lxv	0, 0(6)




	cmpdi	5, 128
	blt	__Process_more_enc


	lxv	32+23, 16(6)
	lxv	32+24, 32(6)
	lxv	32+25, 48(6)
	lxv	32+26, 64(6)
	lxv	32+27, 80(6)
	lxv	32+28, 96(6)
	lxv	32+29, 112(6)
	lxv	32+1, 128(6)


	lwz	23, 240(6)

__Process_encrypt:



__Process_8x_enc:

	li	10, 128
	divdu	12, 5, 10

	addi	12, 12, -1

	vor	15,30,30
	vadduwm	16, 15, 31
	vadduwm	17, 16, 31
	vadduwm	18, 17, 31
	vadduwm	19, 18, 31
	vadduwm	20, 19, 31
	vadduwm	21, 20, 31
	vadduwm	22, 21, 31
	xxlor	9, 32+22, 32+22


	xxlxor	32+15, 32+15, 0
	xxlxor	32+16, 32+16, 0
	xxlxor	32+17, 32+17, 0
	xxlxor	32+18, 32+18, 0
	xxlxor	32+19, 32+19, 0
	xxlxor	32+20, 32+20, 0
	xxlxor	32+21, 32+21, 0
	xxlxor	32+22, 32+22, 0

	li	15, 16
	li	16, 32
	li	17, 48
	li	18, 64
	li	19, 80
	li	20, 96
	li	21, 112





	addi	22, 23, -9
	mtctr	22
	addi	10, 6, 144

.LOOP_8AES_STATE	

__PreLoop_aes_state:
	lxv	32+1, 0(10)
	AES_CIPHER_8x	1
	addi	10, 10, 16
	bdnz	__PreLoop_aes_state
	lxv	32+1, 0(10)

	cmpdi	12, 0
	beq	__Finish_ghash




__Loop_8x_block_enc:
	PROCESS_8X_AES_STATES	


	vxor	15, 15, 0
	PPC_GFMUL128_8x	

	COMPUTE_STATES	

	addi	5, 5, -128
	addi	11, 11, 128

	lxv	32+23, 16(6)
	lxv	32+24, 32(6)
	lxv	32+25, 48(6)
	lxv	32+26, 64(6)
	lxv	32+27, 80(6)
	lxv	32+28, 96(6)
	lxv	32+29, 112(6)
	lxv	32+1, 128(6)



.LOOP_8AES_STATE	
	mtctr	22
	addi	10, 6, 144

__LastLoop_aes_state:
	lxv	32+1, 0(10)
	AES_CIPHER_8x	1
	addi	10, 10, 16
	bdnz	__LastLoop_aes_state

	lxv	32+1, 0(10)

	addi	12, 12, -1
	cmpdi	12, 0
	bne	__Loop_8x_block_enc




__Finish_ghash:
	PROCESS_8X_AES_STATES	


	vxor	15, 15, 0
	PPC_GFMUL128_8x	


	xxlor	30+32, 9, 9
	vadduwm	30, 30, 31
	stxvb16x	32+0, 0, 8

	addi	5, 5, -128
	addi	11, 11, 128





	cmpdi	5, 0
	beq	aes_gcm_out

__Process_more_enc:
	li	24, 1
	bl	aes_gcm_crypt_1x
	cmpdi	5, 0
	beq	aes_gcm_out

	bl	__Process_partial
	b	aes_gcm_out

.size	ppc_aes_gcm_encrypt,.-ppc_aes_gcm_encrypt







.global	ppc_aes_gcm_decrypt
.align	5
ppc_aes_gcm_decrypt:
.localentry	ppc_aes_gcm_decrypt, 0

	SAVE_REGS	
.LOAD_HASH_TABLE	


	lxvb16x	30+32, 0, 7

	mr	14, 3
	mr	9, 4


	vxor	31, 31, 31
	vspltisb	22,1
	vsldoi	31, 31, 22,1

	addis	11, 2, permx@toc@ha
	addi	11, 11, permx@toc@l
	lxv	10, 0(11)
	li	11, 0

	lxv	0, 0(6)




	cmpdi	5, 128
	blt	__Process_more_dec


	lxv	32+23, 16(6)
	lxv	32+24, 32(6)
	lxv	32+25, 48(6)
	lxv	32+26, 64(6)
	lxv	32+27, 80(6)
	lxv	32+28, 96(6)
	lxv	32+29, 112(6)
	lxv	32+1, 128(6)


	lwz	23, 240(6)

__Process_decrypt:



__Process_8x_dec:

	li	10, 128
	divdu	12, 5, 10

	addi	12, 12, -1

	vor	15,30,30
	vadduwm	16, 15, 31
	vadduwm	17, 16, 31
	vadduwm	18, 17, 31
	vadduwm	19, 18, 31
	vadduwm	20, 19, 31
	vadduwm	21, 20, 31
	vadduwm	22, 21, 31
	xxlor	9, 32+22, 32+22


	xxlxor	32+15, 32+15, 0
	xxlxor	32+16, 32+16, 0
	xxlxor	32+17, 32+17, 0
	xxlxor	32+18, 32+18, 0
	xxlxor	32+19, 32+19, 0
	xxlxor	32+20, 32+20, 0
	xxlxor	32+21, 32+21, 0
	xxlxor	32+22, 32+22, 0

	li	15, 16
	li	16, 32
	li	17, 48
	li	18, 64
	li	19, 80
	li	20, 96
	li	21, 112





	addi	22, 23, -9
	mtctr	22
	addi	10, 6, 144

.LOOP_8AES_STATE	

__PreLoop_aes_state_dec:
	lxv	32+1, 0(10)
	AES_CIPHER_8x	1
	addi	10, 10, 16
	bdnz	__PreLoop_aes_state_dec
	lxv	32+1, 0(10)

	cmpdi	12, 0
	beq	__Finish_ghash_dec




__Loop_8x_block_dec:
	.long	0x11EF0D09
	.long	0x12100D09
	.long	0x12310D09
	.long	0x12520D09
	.long	0x12730D09
	.long	0x12940D09
	.long	0x12B50D09
	.long	0x12D60D09

	lxvb16x	32+23, 0, 14
	lxvb16x	32+24, 15, 14
	lxvb16x	32+25, 16, 14
	lxvb16x	32+26, 17, 14
	lxvb16x	32+27, 18, 14
	lxvb16x	32+28, 19, 14
	lxvb16x	32+29, 20, 14
	lxvb16x	32+30, 21, 14
	addi	14, 14, 128

	vxor	15, 15, 23
	vxor	16, 16, 24
	vxor	17, 17, 25
	vxor	18, 18, 26
	vxor	19, 19, 27
	vxor	20, 20, 28
	vxor	21, 21, 29
	vxor	22, 22, 30

	stxvb16x	47, 0, 9
	stxvb16x	48, 15, 9
	stxvb16x	49, 16, 9
	stxvb16x	50, 17, 9
	stxvb16x	51, 18, 9
	stxvb16x	52, 19, 9
	stxvb16x	53, 20, 9
	stxvb16x	54, 21, 9

	addi	9, 9, 128

	vor	15,23,23
	vor	16,24,24
	vor	17,25,25
	vor	18,26,26
	vor	19,27,27
	vor	20,28,28
	vor	21,29,29
	vor	22,30,30


	vxor	15, 15, 0
	PPC_GFMUL128_8x	

	xxlor	32+15, 9, 9
	vadduwm	15, 15, 31
	vadduwm	16, 15, 31
	vadduwm	17, 16, 31
	vadduwm	18, 17, 31
	vadduwm	19, 18, 31
	vadduwm	20, 19, 31
	vadduwm	21, 20, 31
	vadduwm	22, 21, 31
	xxlor	9, 32+22, 32+22

	xxlor	32+27, 0, 0
	vxor	15, 15, 27
	vxor	16, 16, 27
	vxor	17, 17, 27
	vxor	18, 18, 27
	vxor	19, 19, 27
	vxor	20, 20, 27
	vxor	21, 21, 27
	vxor	22, 22, 27

	addi	5, 5, -128
	addi	11, 11, 128

	lxv	32+23, 16(6)
	lxv	32+24, 32(6)
	lxv	32+25, 48(6)
	lxv	32+26, 64(6)
	lxv	32+27, 80(6)
	lxv	32+28, 96(6)
	lxv	32+29, 112(6)
	lxv	32+1, 128(6)

.LOOP_8AES_STATE	
	mtctr	22
	addi	10, 6, 144
__LastLoop_aes_state_dec:
	lxv	32+1, 0(10)
	AES_CIPHER_8x	1
	addi	10, 10, 16
	bdnz	__LastLoop_aes_state_dec
	lxv	32+1, 0(10)

	addi	12, 12, -1
	cmpdi	12, 0
	bne	__Loop_8x_block_dec

__Finish_ghash_dec:
	.long	0x11EF0D09
	.long	0x12100D09
	.long	0x12310D09
	.long	0x12520D09
	.long	0x12730D09
	.long	0x12940D09
	.long	0x12B50D09
	.long	0x12D60D09

	lxvb16x	32+23, 0, 14
	lxvb16x	32+24, 15, 14
	lxvb16x	32+25, 16, 14
	lxvb16x	32+26, 17, 14
	lxvb16x	32+27, 18, 14
	lxvb16x	32+28, 19, 14
	lxvb16x	32+29, 20, 14
	lxvb16x	32+30, 21, 14
	addi	14, 14, 128

	vxor	15, 15, 23
	vxor	16, 16, 24
	vxor	17, 17, 25
	vxor	18, 18, 26
	vxor	19, 19, 27
	vxor	20, 20, 28
	vxor	21, 21, 29
	vxor	22, 22, 30

	stxvb16x	47, 0, 9
	stxvb16x	48, 15, 9
	stxvb16x	49, 16, 9
	stxvb16x	50, 17, 9
	stxvb16x	51, 18, 9
	stxvb16x	52, 19, 9
	stxvb16x	53, 20, 9
	stxvb16x	54, 21, 9
	addi	9, 9, 128

	vxor	15, 23, 0
	vor	16,24,24
	vor	17,25,25
	vor	18,26,26
	vor	19,27,27
	vor	20,28,28
	vor	21,29,29
	vor	22,30,30


	PPC_GFMUL128_8x	

	xxlor	30+32, 9, 9
	vadduwm	30, 30, 31
	stxvb16x	32+0, 0, 8

	addi	5, 5, -128
	addi	11, 11, 128





	cmpdi	5, 0
	beq	aes_gcm_out

__Process_more_dec:
	li	24, 0
	bl	aes_gcm_crypt_1x
	cmpdi	5, 0
	beq	aes_gcm_out

	bl	__Process_partial
	b	aes_gcm_out
.size	ppc_aes_gcm_decrypt,.-ppc_aes_gcm_decrypt

aes_gcm_out:
.localentry	aes_gcm_out,0

	mr	3, 11

	RESTORE_REGS	
	blr	
.size	aes_gcm_out,.-aes_gcm_out

.rodata	
.align	4

permx:
.long	0x4c5d6e7f, 0x08192a3b, 0xc4d5e6f7, 0x8091a2b3
