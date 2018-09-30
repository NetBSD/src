.machine	"any"
.text


.align	5
KeccakF1600_int:
	li	0,24
	mtctr	0
	li	0,0
	b	.Loop

.align	4
.Loop:

	vxor	26,0, 5
	vxor	27,1, 6
	vxor	28,2, 7
	vxor	29,3, 8
	vxor	30,4, 9
.long	0xF3FAD857
.long	0xF35ADB57
.long	0xF37CE857
.long	0xF39CEB57
.long	0xF3BEF257
	vxor	26,26,31
	vxor	27,27,28
	vxor	28,29,30
	vspltisb	31,1
	vxor	26,26,10
	vxor	27,27,11
	vxor	28,28,12

.long	0x13BAF8C4
.long	0x13DBF8C4
.long	0x13FCF8C4
.long	0xF3FFEA57
	vxor	26,26,30
	vxor	27,27,31
	vxor	28,28,29

.long	0xF3BAD057
.long	0xF3DCD257
.long	0xF3FCE357
	vxor	1, 1, 29
	vxor	6, 6, 29
	vxor	10,10,30
	vxor	0, 0, 31
	vxor	5, 5, 31

.long	0xF3BBD857
.long	0xF3DAD357
.long	0xF3FADA57
	vxor	3, 3, 29
	vxor	8, 8, 29
	vxor	2, 2, 30
	vxor	7, 7, 30
	vxor	11,11,31

.long	0xF3BBDB57
	vxor	4, 4, 29
	vxor	9, 9, 29
	vxor	12,12,29


.long	0x134068C4
.long	0x102170C4
.long	0x136278C4
.long	0x138380C4
.long	0x108488C4
.long	0x10A590C4
.long	0x10C698C4
.long	0x13A7A0C4
.long	0x1108A8C4
.long	0x1129B0C4
.long	0x114AB8C4
.long	0x13CBC0C4
.long	0x118CC8C4


.long	0xF01AE057
.long	0xF05D2857
.long	0xF1692957
.long	0xF0A12057
.long	0xF0212357
.long	0xF0683357
.long	0xF08CF257
.long	0xF0E83057
.long	0xF0DBD357
.long	0xF109EB57
.long	0xF18A5357
.long	0xF12AF157
.long	0xF15BE157


.long	0x7FEC0699
	addic	0,0,16

	vandc	26,2, 1
	vandc	27,3, 2
	vandc	28,4, 3
	vandc	29,0, 4
	vandc	30,1, 0
	vxor	0, 0, 26
	vxor	1, 1, 27
	vxor	2, 2, 28
	vxor	3, 3, 29
	vxor	4, 4, 30

	vandc	26,7, 6
	vandc	27,8, 7
	vandc	28,9, 8
	vandc	29,5, 9
	vandc	30,6, 5
	vxor	5, 5, 26
	vxor	6, 6, 27
	vxor	7, 7, 28
	vxor	8, 8, 29
	vxor	9, 9, 30

	vxor	0, 0, 31

.long	0xF34A5A57
.long	0xF36C5057
.long	0xF38B6257
.long	0xF3AA5257
	vandc	26,11,26
	vandc	27,27,28
	vandc	28,10,29
	vxor	10,10,26
	vxor	11,11,27
	vxor	12,12,28

	bc	16,0,.Loop

.long	0xF18C6357
	blr	
.long	0
.byte	0,12,0x14,0,0,0,0,0



.align	5
KeccakF1600:
	stdu	1,-256(1)
	li	10,63
	li	11,79
	mflr	8
	mfspr	7,256
	stvx	20,10,1
	addi	10,10,32
	stvx	21,11,1
	addi	11,11,32
	stvx	22,10,1
	addi	10,10,32
	stvx	23,11,1
	addi	11,11,32
	stvx	24,10,1
	addi	10,10,32
	stvx	25,11,1
	addi	11,11,32
	stvx	26,10,1
	addi	10,10,32
	stvx	27,11,1
	addi	11,11,32
	stvx	28,10,1
	addi	10,10,32
	stvx	29,11,1
	addi	11,11,32
	stvx	30,10,1
	stvx	31,11,1
	stw	7,252(1)
	li	0, -1
	std	8,272(1)
	mtspr	256,0

	li	11,16
.long	0x7C001E19
	li	10,32
.long	0x7C2B1E19
	addi	11,11,32
.long	0x7C4A1E19
	addi	10,10,32
.long	0x7C6B1E19
	addi	11,11,32
.long	0x7C8A1E19
	addi	10,10,32
.long	0x7CAB1E19
	addi	11,11,32
.long	0x7CCA1E19
	addi	10,10,32
.long	0x7CEB1E19
	addi	11,11,32
.long	0x7D0A1E19
	addi	10,10,32
.long	0x7D2B1E19
	addi	11,11,32
.long	0x7D4A1E19
	addi	10,10,32
.long	0x7D6B1E19
.long	0x7D8A1A99

	bl	PICmeup

	li	11,16
.long	0x7DA06699
	li	10,32
.long	0x7DCB6699
	addi	11,11,32
.long	0x7DEA6699
	addi	10,10,32
.long	0x7E0B6699
	addi	11,11,32
.long	0x7E2A6699
	addi	10,10,32
.long	0x7E4B6699
	addi	11,11,32
.long	0x7E6A6699
	addi	10,10,32
.long	0x7E8B6699
	addi	11,11,32
.long	0x7EAA6699
	addi	10,10,32
.long	0x7ECB6699
	addi	11,11,32
.long	0x7EEA6699
	addi	10,10,32
.long	0x7F0B6699
.long	0x7F2A6699
	addi	12,12,256

	bl	KeccakF1600_int

	li	11,16
.long	0x7C001F19
	li	10,32
.long	0x7C2B1F19
	addi	11,11,32
.long	0x7C4A1F19
	addi	10,10,32
.long	0x7C6B1F19
	addi	11,11,32
.long	0x7C8A1F19
	addi	10,10,32
.long	0x7CAB1F19
	addi	11,11,32
.long	0x7CCA1F19
	addi	10,10,32
.long	0x7CEB1F19
	addi	11,11,32
.long	0x7D0A1F19
	addi	10,10,32
.long	0x7D2B1F19
	addi	11,11,32
.long	0x7D4A1F19
	addi	10,10,32
.long	0x7D6B1F19
.long	0x7D8A1D99

	li	10,63
	li	11,79
	mtlr	8
	mtspr	256,7
	lvx	20,10,1
	addi	10,10,32
	lvx	21,11,1
	addi	11,11,32
	lvx	22,10,1
	addi	10,10,32
	lvx	23,11,1
	addi	11,11,32
	lvx	24,10,1
	addi	10,10,32
	lvx	25,11,1
	addi	11,11,32
	lvx	26,10,1
	addi	10,10,32
	lvx	27,11,1
	addi	11,11,32
	lvx	28,10,1
	addi	10,10,32
	lvx	29,11,1
	addi	11,11,32
	lvx	30,10,1
	lvx	31,11,1
	addi	1,1,256
	blr	
.long	0
.byte	0,12,0x04,1,0x80,0,1,0
.long	0

.globl	SHA3_absorb
.type	SHA3_absorb,@function
.section	".opd","aw"
.align	3
SHA3_absorb:
.quad	.SHA3_absorb,.TOC.@tocbase,0
.previous

.align	5
.SHA3_absorb:
	stdu	1,-256(1)
	li	10,63
	li	11,79
	mflr	8
	mfspr	7,256
	stvx	20,10,1
	addi	10,10,32
	stvx	21,11,1
	addi	11,11,32
	stvx	22,10,1
	addi	10,10,32
	stvx	23,11,1
	addi	11,11,32
	stvx	24,10,1
	addi	10,10,32
	stvx	25,11,1
	addi	11,11,32
	stvx	26,10,1
	addi	10,10,32
	stvx	27,11,1
	addi	11,11,32
	stvx	28,10,1
	addi	10,10,32
	stvx	29,11,1
	addi	11,11,32
	stvx	30,10,1
	stvx	31,11,1
	stw	7,252(1)
	li	0, -1
	std	8,272(1)
	mtspr	256,0

	li	11,16
.long	0x7C001E19
	li	10,32
.long	0x7C2B1E19
	addi	11,11,32
.long	0x7C4A1E19
	addi	10,10,32
.long	0x7C6B1E19
	addi	11,11,32
.long	0x7C8A1E19
	addi	10,10,32
.long	0x7CAB1E19
	addi	11,11,32
.long	0x7CCA1E19
	addi	10,10,32
.long	0x7CEB1E19
	addi	11,11,32
.long	0x7D0A1E19
	addi	10,10,32
.long	0x7D2B1E19
	addi	11,11,32
.long	0x7D4A1E19
	addi	10,10,32
.long	0x7D6B1E19
.long	0x7D8A1A99

	bl	PICmeup

	li	11,16
.long	0x7DA06699
	li	10,32
.long	0x7DCB6699
	addi	11,11,32
.long	0x7DEA6699
	addi	10,10,32
.long	0x7E0B6699
	addi	11,11,32
.long	0x7E2A6699
	addi	10,10,32
.long	0x7E4B6699
	addi	11,11,32
.long	0x7E6A6699
	addi	10,10,32
.long	0x7E8B6699
	addi	11,11,32
.long	0x7EAA6699
	addi	10,10,32
.long	0x7ECB6699
	addi	11,11,32
.long	0x7EEA6699
	addi	10,10,32
.long	0x7F0B6699
.long	0x7F2A6699
	li	10,-32
	li	11,-16
	addi	12,12,256
	b	.Loop_absorb

.align	4
.Loop_absorb:
	cmpld	5,6
	blt	.Labsorbed

	sub	5,5,6
	srwi	0,6,3
	mtctr	0

.long	0x7FCA6699
.long	0x7FEB6699
	vspltisb	27,7
	vxor	30,30,27
	vxor	31,31,27

	vxor	27,27,27
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	0, 0, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	1, 1, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	2, 2, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	3, 3, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	4, 4, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	0, 0, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	1, 1, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	2, 2, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	3, 3, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	4, 4, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	5, 5, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	6, 6, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	7, 7, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	8, 8, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	9, 9, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	5, 5, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	6, 6, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	7, 7, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	8, 8, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	9, 9, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	10, 10, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	10, 10, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,30
	vxor	11, 11, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	11, 11, 26
	bdz	.Lprocess_block
.long	0x7F402499
	addi	4,4,8
	vperm	26,26,27,31
	vxor	12, 12, 26

.Lprocess_block:
	bl	KeccakF1600_int

	b	.Loop_absorb

.align	4
.Labsorbed:
	li	11,16
.long	0x7C001F19
	li	10,32
.long	0x7C2B1F19
	addi	11,11,32
.long	0x7C4A1F19
	addi	10,10,32
.long	0x7C6B1F19
	addi	11,11,32
.long	0x7C8A1F19
	addi	10,10,32
.long	0x7CAB1F19
	addi	11,11,32
.long	0x7CCA1F19
	addi	10,10,32
.long	0x7CEB1F19
	addi	11,11,32
.long	0x7D0A1F19
	addi	10,10,32
.long	0x7D2B1F19
	addi	11,11,32
.long	0x7D4A1F19
	addi	10,10,32
.long	0x7D6B1F19
.long	0x7D8A1D99

	mr	3,5
	li	10,63
	li	11,79
	mtlr	8
	mtspr	256,7
	lvx	20,10,1
	addi	10,10,32
	lvx	21,11,1
	addi	11,11,32
	lvx	22,10,1
	addi	10,10,32
	lvx	23,11,1
	addi	11,11,32
	lvx	24,10,1
	addi	10,10,32
	lvx	25,11,1
	addi	11,11,32
	lvx	26,10,1
	addi	10,10,32
	lvx	27,11,1
	addi	11,11,32
	lvx	28,10,1
	addi	10,10,32
	lvx	29,11,1
	addi	11,11,32
	lvx	30,10,1
	lvx	31,11,1
	addi	1,1,256
	blr	
.long	0
.byte	0,12,0x04,1,0x80,0,4,0
.long	0

.globl	SHA3_squeeze
.type	SHA3_squeeze,@function
.section	".opd","aw"
.align	3
SHA3_squeeze:
.quad	.SHA3_squeeze,.TOC.@tocbase,0
.previous

.align	5
.SHA3_squeeze:
	mflr	9
	subi	4,4,1
	addi	8,3,4
	mr	10,6
	li	11,0
	b	.Loop_squeeze
.align	4
.Loop_squeeze:
	lwzx	7,11,8
	lwzx	0,11,3
	cmpldi	5,8
	blt	.Lsqueeze_tail

	stbu	7,1(4)
	srwi	7,7,8
	stbu	7,1(4)
	srwi	7,7,8
	stbu	7,1(4)
	srwi	7,7,8
	stbu	7,1(4)
	stbu	0,1(4)
	srwi	0,0,8
	stbu	0,1(4)
	srwi	0,0,8
	stbu	0,1(4)
	srwi	0,0,8
	stbu	0,1(4)

	subic.	5,5,8
	bclr	12,2

	subic.	10,10,8
	ble	.Loutput_expand

	addi	11,11,16
	cmplwi	11,80
	blt	.Loop_squeeze
	subi	11,11,72
	beq	.Loop_squeeze
	addi	11,11,72
	cmplwi	11,88
	subi	11,11,8
	beq	.Loop_squeeze
	addi	11,11,8
	cmplwi	11,160
	subi	11,11,72
	beq	.Loop_squeeze
	addi	11,11,72
	blt	.Loop_squeeze
	subi	11,11,8
	b	.Loop_squeeze

.align	4
.Loutput_expand:
	bl	KeccakF1600
	mtlr	9

	addi	8,3,4
	mr	10,6
	li	11,0
	b	.Loop_squeeze

.align	4
.Lsqueeze_tail:
	mtctr	5
	subic.	5,5,4
	ble	.Loop_tail_lo
	li	8,4
	mtctr	8
.Loop_tail_lo:
	stbu	7,1(4)
	srdi	7,7,8
	bc	16,0,.Loop_tail_lo
	ble	.Lsqueeze_done
	mtctr	5
.Loop_tail_hi:
	stbu	0,1(4)
	srdi	0,0,8
	bc	16,0,.Loop_tail_hi

.Lsqueeze_done:
	blr	
.long	0
.byte	0,12,0x14,0,0,0,4,0
.long	0

.align	6
PICmeup:
	mflr	0
	bcl	20,31,$+4
	mflr	12
	addi	12,12,56
	mtlr	0
	blr	
.long	0
.byte	0,12,0x14,0,0,0,0,0
.space	28

.align	6
rhotates:
.long	0,0
.long	0,36
.long	0,1
.long	0,44
.long	0,62
.long	0,6
.long	0,28
.long	0,55
.long	0,27
.long	0,20
.long	0,3
.long	0,41
.long	0,10
.long	0,45
.long	0,43
.long	0,15
.long	0,25
.long	0,21
.long	0,39
.long	0,8
.long	0,18
.long	0,2
.long	0,61
.long	0,56
.long	0,14
.long	0,14

.long	0,0
.long	0,0
.long	0x00010203,0x04050607
.long	0x10111213,0x14151617
.long	0x10111213,0x14151617
.long	0x00010203,0x04050607

iotas:
.long	0x00000000,0x00000001
.long	0,0
.long	0x00000000,0x00008082
.long	0,0
.long	0x80000000,0x0000808a
.long	0,0
.long	0x80000000,0x80008000
.long	0,0
.long	0x00000000,0x0000808b
.long	0,0
.long	0x00000000,0x80000001
.long	0,0
.long	0x80000000,0x80008081
.long	0,0
.long	0x80000000,0x00008009
.long	0,0
.long	0x00000000,0x0000008a
.long	0,0
.long	0x00000000,0x00000088
.long	0,0
.long	0x00000000,0x80008009
.long	0,0
.long	0x00000000,0x8000000a
.long	0,0
.long	0x00000000,0x8000808b
.long	0,0
.long	0x80000000,0x0000008b
.long	0,0
.long	0x80000000,0x00008089
.long	0,0
.long	0x80000000,0x00008003
.long	0,0
.long	0x80000000,0x00008002
.long	0,0
.long	0x80000000,0x00000080
.long	0,0
.long	0x00000000,0x0000800a
.long	0,0
.long	0x80000000,0x8000000a
.long	0,0
.long	0x80000000,0x80008081
.long	0,0
.long	0x80000000,0x00008080
.long	0,0
.long	0x00000000,0x80000001
.long	0,0
.long	0x80000000,0x80008008
.long	0,0

.byte	75,101,99,99,97,107,45,49,54,48,48,32,97,98,115,111,114,98,32,97,110,100,32,115,113,117,101,101,122,101,32,102,111,114,32,80,111,119,101,114,73,83,65,32,50,46,48,55,44,67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115,108,46,111,114,103,62,0
.align	2
