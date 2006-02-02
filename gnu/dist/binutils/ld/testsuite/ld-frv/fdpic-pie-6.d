#name: FRV uClinux PIC relocs to weak undefined symbols, pie linking
#source: fdpic6.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -pie --defsym WD1=D6

.*:     file format elf.*frv.*

Disassembly of section \.plt:

000004c8 <\.plt>:
 4c8:	00 00 00 08 	add\.p gr0,gr8,gr0
 4cc:	c0 1a 00 06 	bra 4e4 <F6-0x10>
 4d0:	00 00 00 00 	add\.p gr0,gr0,gr0
 4d4:	c0 1a 00 04 	bra 4e4 <F6-0x10>
 4d8:	00 00 00 10 	add\.p gr0,gr16,gr0
 4dc:	c0 1a 00 02 	bra 4e4 <F6-0x10>
 4e0:	00 00 00 18 	add\.p gr0,gr24,gr0
 4e4:	88 08 f1 40 	ldd @\(gr15,gr0\),gr4
 4e8:	80 30 40 00 	jmpl @\(gr4,gr0\)
 4ec:	9c cc ff f0 	lddi @\(gr15,-16\),gr14
 4f0:	80 30 e0 00 	jmpl @\(gr14,gr0\)
Disassembly of section \.text:

000004f4 <F6>:
 4f4:	fe 3f ff fe 	call 4ec <F6-0x8>
 4f8:	80 40 f0 0c 	addi gr15,12,gr0
 4fc:	80 fc 00 24 	setlos 0x24,gr0
 500:	80 f4 00 20 	setlo 0x20,gr0
 504:	80 f8 00 00 	sethi hi\(0x0\),gr0
 508:	80 40 f0 10 	addi gr15,16,gr0
 50c:	80 fc 00 18 	setlos 0x18,gr0
 510:	80 f4 00 1c 	setlo 0x1c,gr0
 514:	80 f8 00 00 	sethi hi\(0x0\),gr0
 518:	80 40 ff f8 	addi gr15,-8,gr0
 51c:	80 fc ff e8 	setlos 0xf*ffffffe8,gr0
 520:	80 f4 ff e0 	setlo 0xffe0,gr0
 524:	80 f8 ff ff 	sethi 0xffff,gr0
 528:	80 f4 ff d4 	setlo 0xffd4,gr0
 52c:	80 f8 ff ff 	sethi 0xffff,gr0
 530:	80 f4 00 14 	setlo 0x14,gr0
 534:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

000045cc <D6>:
	\.\.\.
			45cc: R_FRV_32	WD0
			45d0: R_FRV_FUNCDESC	WFb
			45d4: R_FRV_32	WFb
Disassembly of section \.got:

000045d8 <_GLOBAL_OFFSET_TABLE_-0x20>:
    45d8:	00 00 04 e4 	addxcc\.p gr0,gr36,gr0,icc1
			45d8: R_FRV_FUNCDESC_VALUE	WF9
    45dc:	00 00 00 02 	add\.p gr0,fp,gr0
    45e0:	00 00 04 dc 	addxcc\.p gr0,gr28,gr0,icc1
			45e0: R_FRV_FUNCDESC_VALUE	WF8
    45e4:	00 00 00 02 	add\.p gr0,fp,gr0
    45e8:	00 00 04 d4 	addxcc\.p gr0,gr20,gr0,icc1
			45e8: R_FRV_FUNCDESC_VALUE	WF0
    45ec:	00 00 00 02 	add\.p gr0,fp,gr0
    45f0:	00 00 04 cc 	addxcc\.p gr0,gr12,gr0,icc1
			45f0: R_FRV_FUNCDESC_VALUE	WF7
    45f4:	00 00 00 02 	add\.p gr0,fp,gr0

000045f8 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			4604: R_FRV_32	WF1
			4608: R_FRV_FUNCDESC	WF4
			460c: R_FRV_32	WD2
			4610: R_FRV_FUNCDESC	WF5
			4614: R_FRV_FUNCDESC	WF6
			4618: R_FRV_32	WF3
			461c: R_FRV_32	WF2
