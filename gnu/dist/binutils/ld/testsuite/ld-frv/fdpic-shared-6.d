#name: FRV uClinux PIC relocs to weak undefined symbols, shared linking
#source: fdpic6.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -shared --defsym WD1=D6 --version-script fdpic6.ldv

.*:     file format elf.*frv.*

Disassembly of section \.plt:

0000037c <\.plt>:
 37c:	00 00 00 08 	add\.p gr0,gr8,gr0
 380:	c0 1a 00 06 	bra 398 <F6-0x10>
 384:	00 00 00 00 	add\.p gr0,gr0,gr0
 388:	c0 1a 00 04 	bra 398 <F6-0x10>
 38c:	00 00 00 10 	add\.p gr0,gr16,gr0
 390:	c0 1a 00 02 	bra 398 <F6-0x10>
 394:	00 00 00 18 	add\.p gr0,gr24,gr0
 398:	88 08 f1 40 	ldd @\(gr15,gr0\),gr4
 39c:	80 30 40 00 	jmpl @\(gr4,gr0\)
 3a0:	9c cc ff f0 	lddi @\(gr15,-16\),gr14
 3a4:	80 30 e0 00 	jmpl @\(gr14,gr0\)
Disassembly of section \.text:

000003a8 <F6>:
 3a8:	fe 3f ff fe 	call 3a0 <F6-0x8>
 3ac:	80 40 f0 0c 	addi gr15,12,gr0
 3b0:	80 fc 00 24 	setlos 0x24,gr0
 3b4:	80 f4 00 20 	setlo 0x20,gr0
 3b8:	80 f8 00 00 	sethi hi\(0x0\),gr0
 3bc:	80 40 f0 10 	addi gr15,16,gr0
 3c0:	80 fc 00 18 	setlos 0x18,gr0
 3c4:	80 f4 00 1c 	setlo 0x1c,gr0
 3c8:	80 f8 00 00 	sethi hi\(0x0\),gr0
 3cc:	80 40 ff f8 	addi gr15,-8,gr0
 3d0:	80 fc ff e8 	setlos 0xf*ffffffe8,gr0
 3d4:	80 f4 ff e0 	setlo 0xffe0,gr0
 3d8:	80 f8 ff ff 	sethi 0xffff,gr0
 3dc:	80 f4 ff d0 	setlo 0xffd0,gr0
 3e0:	80 f8 ff ff 	sethi 0xffff,gr0
 3e4:	80 f4 00 14 	setlo 0x14,gr0
 3e8:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

00004480 <D6>:
	\.\.\.
			4480: R_FRV_32	WD0
			4484: R_FRV_FUNCDESC	WFb
			4488: R_FRV_32	WFb
Disassembly of section \.got:

00004490 <_GLOBAL_OFFSET_TABLE_-0x20>:
    4490:	00 00 03 98 	sdiv\.p gr0,gr24,gr0
			4490: R_FRV_FUNCDESC_VALUE	WF9
    4494:	00 00 00 00 	add\.p gr0,gr0,gr0
    4498:	00 00 03 90 	sdiv\.p gr0,gr16,gr0
			4498: R_FRV_FUNCDESC_VALUE	WF8
    449c:	00 00 00 00 	add\.p gr0,gr0,gr0
    44a0:	00 00 03 88 	sdiv\.p gr0,gr8,gr0
			44a0: R_FRV_FUNCDESC_VALUE	WF0
    44a4:	00 00 00 00 	add\.p gr0,gr0,gr0
    44a8:	00 00 03 80 	sdiv\.p gr0,gr0,gr0
			44a8: R_FRV_FUNCDESC_VALUE	WF7
    44ac:	00 00 00 00 	add\.p gr0,gr0,gr0

000044b0 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			44bc: R_FRV_32	WF1
			44c0: R_FRV_FUNCDESC	WF4
			44c4: R_FRV_32	WD2
			44c8: R_FRV_FUNCDESC	WF5
			44cc: R_FRV_FUNCDESC	WF6
			44d0: R_FRV_32	WF3
			44d4: R_FRV_32	WF2
