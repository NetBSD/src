#source: tlstoc.s
#as: -a64
#ld: -shared -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

0+6b8 <\.__tls_get_addr>:
 6b8:	3d 82 00 00 	addis	r12,r2,0
 6bc:	f8 41 00 28 	std	r2,40\(r1\)
 6c0:	e9 6c 80 70 	ld	r11,-32656\(r12\)
 6c4:	e8 4c 80 78 	ld	r2,-32648\(r12\)
 6c8:	7d 69 03 a6 	mtctr	r11
 6cc:	e9 6c 80 80 	ld	r11,-32640\(r12\)
 6d0:	4e 80 04 20 	bctr

0+6d4 <_start>:
 6d4:	38 62 80 08 	addi	r3,r2,-32760
 6d8:	4b ff ff e1 	bl	6b8 <\.__tls_get_addr>
 6dc:	e8 41 00 28 	ld	r2,40\(r1\)
 6e0:	38 62 80 18 	addi	r3,r2,-32744
 6e4:	4b ff ff d5 	bl	6b8 <\.__tls_get_addr>
 6e8:	e8 41 00 28 	ld	r2,40\(r1\)
 6ec:	38 62 80 28 	addi	r3,r2,-32728
 6f0:	4b ff ff c9 	bl	6b8 <\.__tls_get_addr>
 6f4:	e8 41 00 28 	ld	r2,40\(r1\)
 6f8:	38 62 80 38 	addi	r3,r2,-32712
 6fc:	4b ff ff bd 	bl	6b8 <\.__tls_get_addr>
 700:	e8 41 00 28 	ld	r2,40\(r1\)
 704:	39 23 80 40 	addi	r9,r3,-32704
 708:	3d 23 00 00 	addis	r9,r3,0
 70c:	81 49 80 48 	lwz	r10,-32696\(r9\)
 710:	3d 2d 00 00 	addis	r9,r13,0
 714:	7d 49 18 2a 	ldx	r10,r9,r3
 718:	e9 22 80 50 	ld	r9,-32688\(r2\)
 71c:	7d 49 6a 2e 	lhzx	r10,r9,r13
 720:	89 4d 00 00 	lbz	r10,0\(r13\)
 724:	3d 2d 00 00 	addis	r9,r13,0
 728:	99 49 00 00 	stb	r10,0\(r9\)
 72c:	e8 41 00 28 	ld	r2,40\(r1\)
 730:	3d 82 00 00 	addis	r12,r2,0
 734:	e9 6c 80 58 	ld	r11,-32680\(r12\)
 738:	e8 4c 80 60 	ld	r2,-32672\(r12\)
 73c:	7d 69 03 a6 	mtctr	r11
 740:	e9 6c 80 68 	ld	r11,-32664\(r12\)
 744:	4e 80 04 20 	bctr
 748:	60 00 00 00 	nop
 74c:	38 00 00 00 	li	r0,0
 750:	4b ff ff dc 	b	72c <_start\+0x58>
