#name: FRV uClinux PIC relocs to global symbols, pie linking
#source: fdpic2.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -pie

.*:     file format elf.*frv.*

Disassembly of section \.text:

000004f0 <F2>:
 4f0:	80 3c 00 01 	call 4f4 <GF0>

000004f4 <GF0>:
 4f4:	80 40 f0 10 	addi gr15,16,gr0
 4f8:	80 fc 00 24 	setlos 0x24,gr0
 4fc:	80 f4 00 20 	setlo 0x20,gr0
 500:	80 f8 00 00 	sethi hi\(0x0\),gr0
 504:	80 40 f0 0c 	addi gr15,12,gr0
 508:	80 fc 00 18 	setlos 0x18,gr0
 50c:	80 f4 00 14 	setlo 0x14,gr0
 510:	80 f8 00 00 	sethi hi\(0x0\),gr0
 514:	80 40 ff f8 	addi gr15,-8,gr0
 518:	80 fc ff f0 	setlos 0xf*fffffff0,gr0
 51c:	80 f4 ff e8 	setlo 0xffe8,gr0
 520:	80 f8 ff ff 	sethi 0xffff,gr0
 524:	80 40 ff dc 	addi gr15,-36,gr0
 528:	80 fc ff dc 	setlos 0xf*ffffffdc,gr0
 52c:	80 f4 ff dc 	setlo 0xffdc,gr0
 530:	80 f8 ff ff 	sethi 0xffff,gr0
 534:	80 f4 00 1c 	setlo 0x1c,gr0
 538:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

000045b8 <D2>:
    45b8:	00 00 00 04 	add\.p gr0,gr4,gr0
			45b8: R_FRV_32	\.data

000045bc <GD0>:
    45bc:	00 00 00 04 	add\.p gr0,gr4,gr0
			45bc: R_FRV_FUNCDESC	\.text
    45c0:	00 00 00 04 	add\.p gr0,gr4,gr0
			45c0: R_FRV_32	\.text
Disassembly of section \.got:

000045c8 <_GLOBAL_OFFSET_TABLE_-0x18>:
    45c8:	00 00 00 04 	add\.p gr0,gr4,gr0
			45c8: R_FRV_FUNCDESC_VALUE	\.text
    45cc:	00 00 00 02 	add\.p gr0,fp,gr0
    45d0:	00 00 00 04 	add\.p gr0,gr4,gr0
			45d0: R_FRV_FUNCDESC_VALUE	\.text
    45d4:	00 00 00 02 	add\.p gr0,fp,gr0
    45d8:	00 00 00 04 	add\.p gr0,gr4,gr0
			45d8: R_FRV_FUNCDESC_VALUE	\.text
    45dc:	00 00 00 02 	add\.p gr0,fp,gr0

000045e0 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    45ec:	00 00 00 04 	add\.p gr0,gr4,gr0
			45ec: R_FRV_FUNCDESC	\.text
    45f0:	00 00 00 04 	add\.p gr0,gr4,gr0
			45f0: R_FRV_32	\.text
    45f4:	00 00 00 04 	add\.p gr0,gr4,gr0
			45f4: R_FRV_FUNCDESC	\.text
    45f8:	00 00 00 04 	add\.p gr0,gr4,gr0
			45f8: R_FRV_FUNCDESC	\.text
    45fc:	00 00 00 04 	add\.p gr0,gr4,gr0
			45fc: R_FRV_32	\.data
    4600:	00 00 00 04 	add\.p gr0,gr4,gr0
			4600: R_FRV_32	\.text
    4604:	00 00 00 04 	add\.p gr0,gr4,gr0
			4604: R_FRV_32	\.text
