#name: FRV uClinux PIC relocs to (mostly) global symbols, shared linking
#source: fdpic2.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -shared --version-script fdpic2min.ldv

.*:     file format elf.*frv.*

Disassembly of section \.plt:

000004d8 <\.plt>:
 4d8:	00 00 00 00 	add\.p gr0,gr0,gr0
 4dc:	c0 1a 00 06 	bra 4f4 <F2-0x10>
 4e0:	00 00 00 10 	add\.p gr0,gr16,gr0
 4e4:	c0 1a 00 04 	bra 4f4 <F2-0x10>
 4e8:	00 00 00 18 	add\.p gr0,gr24,gr0
 4ec:	c0 1a 00 02 	bra 4f4 <F2-0x10>
 4f0:	00 00 00 08 	add\.p gr0,gr8,gr0
 4f4:	88 08 f1 40 	ldd @\(gr15,gr0\),gr4
 4f8:	80 30 40 00 	jmpl @\(gr4,gr0\)
 4fc:	9c cc ff f8 	lddi @\(gr15,-8\),gr14
 500:	80 30 e0 00 	jmpl @\(gr14,gr0\)
Disassembly of section \.text:

00000504 <F2>:
 504:	fe 3f ff fe 	call 4fc <F2-0x8>

00000508 <GF0>:
 508:	80 40 f0 10 	addi gr15,16,gr0
 50c:	80 fc 00 24 	setlos 0x24,gr0
 510:	80 f4 00 20 	setlo 0x20,gr0
 514:	80 f8 00 00 	sethi hi\(0x0\),gr0
 518:	80 40 f0 0c 	addi gr15,12,gr0
 51c:	80 fc 00 18 	setlos 0x18,gr0
 520:	80 f4 00 14 	setlo 0x14,gr0
 524:	80 f8 00 00 	sethi hi\(0x0\),gr0
 528:	80 40 ff f0 	addi gr15,-16,gr0
 52c:	80 fc ff e8 	setlos 0xf*ffffffe8,gr0
 530:	80 f4 ff e0 	setlo 0xffe0,gr0
 534:	80 f8 ff ff 	sethi 0xffff,gr0
 538:	80 40 ff d8 	addi gr15,-40,gr0
 53c:	80 fc ff d8 	setlos 0xf*ffffffd8,gr0
 540:	80 f4 ff d8 	setlo 0xffd8,gr0
 544:	80 f8 ff ff 	sethi 0xffff,gr0
 548:	80 f4 00 1c 	setlo 0x1c,gr0
 54c:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

000045e4 <D2>:
    45e4:	00 00 00 00 	add\.p gr0,gr0,gr0
			45e4: R_FRV_32	GD0

000045e8 <GD0>:
	\.\.\.
			45e8: R_FRV_FUNCDESC	GFb
			45ec: R_FRV_32	GFb
Disassembly of section \.got:

000045f0 <_GLOBAL_OFFSET_TABLE_-0x20>:
    45f0:	00 00 04 ec 	addxcc\.p gr0,gr44,gr0,icc1
			45f0: R_FRV_FUNCDESC_VALUE	GF9
    45f4:	00 00 00 00 	add\.p gr0,gr0,gr0
    45f8:	00 00 04 e4 	addxcc\.p gr0,gr36,gr0,icc1
			45f8: R_FRV_FUNCDESC_VALUE	GF8
    45fc:	00 00 00 00 	add\.p gr0,gr0,gr0
    4600:	00 00 04 f4 	addxcc\.p gr0,gr52,gr0,icc1
			4600: R_FRV_FUNCDESC_VALUE	GF7
    4604:	00 00 00 00 	add\.p gr0,gr0,gr0
    4608:	00 00 04 dc 	addxcc\.p gr0,gr28,gr0,icc1
			4608: R_FRV_FUNCDESC_VALUE	GF0
    460c:	00 00 00 00 	add\.p gr0,gr0,gr0

00004610 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			461c: R_FRV_FUNCDESC	GF4
			4620: R_FRV_32	GF1
			4624: R_FRV_FUNCDESC	GF6
			4628: R_FRV_FUNCDESC	GF5
			462c: R_FRV_32	GD4
			4630: R_FRV_32	GF3
			4634: R_FRV_32	GF2
