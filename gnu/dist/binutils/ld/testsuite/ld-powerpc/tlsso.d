#source: tls.s
#as: -a64
#ld: -shared -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

0+718 <\.__tls_get_addr>:
 718:	3d 82 00 00 	addis	r12,r2,0
 71c:	f8 41 00 28 	std	r2,40\(r1\)
 720:	e9 6c 80 78 	ld	r11,-32648\(r12\)
 724:	e8 4c 80 80 	ld	r2,-32640\(r12\)
 728:	7d 69 03 a6 	mtctr	r11
 72c:	e9 6c 80 88 	ld	r11,-32632\(r12\)
 730:	4e 80 04 20 	bctr

0+734 <_start>:
 734:	38 62 80 30 	addi	r3,r2,-32720
 738:	4b ff ff e1 	bl	718 <\.__tls_get_addr>
 73c:	e8 41 00 28 	ld	r2,40\(r1\)
 740:	38 62 80 08 	addi	r3,r2,-32760
 744:	4b ff ff d5 	bl	718 <\.__tls_get_addr>
 748:	e8 41 00 28 	ld	r2,40\(r1\)
 74c:	38 62 80 48 	addi	r3,r2,-32696
 750:	4b ff ff c9 	bl	718 <\.__tls_get_addr>
 754:	e8 41 00 28 	ld	r2,40\(r1\)
 758:	38 62 80 08 	addi	r3,r2,-32760
 75c:	4b ff ff bd 	bl	718 <\.__tls_get_addr>
 760:	e8 41 00 28 	ld	r2,40\(r1\)
 764:	39 23 80 40 	addi	r9,r3,-32704
 768:	3d 23 00 00 	addis	r9,r3,0
 76c:	81 49 80 48 	lwz	r10,-32696\(r9\)
 770:	e9 22 80 40 	ld	r9,-32704\(r2\)
 774:	7d 49 18 2a 	ldx	r10,r9,r3
 778:	e9 22 80 58 	ld	r9,-32680\(r2\)
 77c:	7d 49 6a 2e 	lhzx	r10,r9,r13
 780:	89 4d 00 00 	lbz	r10,0\(r13\)
 784:	3d 2d 00 00 	addis	r9,r13,0
 788:	99 49 00 00 	stb	r10,0\(r9\)
 78c:	38 62 80 18 	addi	r3,r2,-32744
 790:	4b ff ff 89 	bl	718 <\.__tls_get_addr>
 794:	e8 41 00 28 	ld	r2,40\(r1\)
 798:	38 62 80 08 	addi	r3,r2,-32760
 79c:	4b ff ff 7d 	bl	718 <\.__tls_get_addr>
 7a0:	e8 41 00 28 	ld	r2,40\(r1\)
 7a4:	f9 43 80 08 	std	r10,-32760\(r3\)
 7a8:	3d 23 00 00 	addis	r9,r3,0
 7ac:	91 49 80 10 	stw	r10,-32752\(r9\)
 7b0:	e9 22 80 28 	ld	r9,-32728\(r2\)
 7b4:	7d 49 19 2a 	stdx	r10,r9,r3
 7b8:	e9 22 80 58 	ld	r9,-32680\(r2\)
 7bc:	7d 49 6b 2e 	sthx	r10,r9,r13
 7c0:	e9 4d 90 2a 	lwa	r10,-28632\(r13\)
 7c4:	3d 2d 00 00 	addis	r9,r13,0
 7c8:	a9 49 90 30 	lha	r10,-28624\(r9\)
 7cc:	e8 41 00 28 	ld	r2,40\(r1\)
 7d0:	3d 82 00 00 	addis	r12,r2,0
 7d4:	e9 6c 80 60 	ld	r11,-32672\(r12\)
 7d8:	e8 4c 80 68 	ld	r2,-32664\(r12\)
 7dc:	7d 69 03 a6 	mtctr	r11
 7e0:	e9 6c 80 70 	ld	r11,-32656\(r12\)
 7e4:	4e 80 04 20 	bctr
 7e8:	60 00 00 00 	nop
 7ec:	38 00 00 00 	li	r0,0
 7f0:	4b ff ff dc 	b	7cc <_start\+0x98>
