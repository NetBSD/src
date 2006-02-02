#name: FRV uClinux PIC relocs to protected symbols, shared linking
#source: fdpic4.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -shared

.*:     file format elf.*frv.*

Disassembly of section \.text:

00000564 <F4>:
 564:	80 3c 00 01 	call 568 <PF0>

00000568 <PF0>:
 568:	80 40 f0 10 	addi gr15,16,gr0
 56c:	80 fc 00 20 	setlos 0x20,gr0
 570:	80 f4 00 1c 	setlo 0x1c,gr0
 574:	80 f8 00 00 	sethi hi\(0x0\),gr0
 578:	80 40 f0 0c 	addi gr15,12,gr0
 57c:	80 fc 00 24 	setlos 0x24,gr0
 580:	80 f4 00 18 	setlo 0x18,gr0
 584:	80 f8 00 00 	sethi hi\(0x0\),gr0
 588:	80 40 ff f8 	addi gr15,-8,gr0
 58c:	80 fc ff f0 	setlos 0xf*fffffff0,gr0
 590:	80 f4 ff e8 	setlo 0xffe8,gr0
 594:	80 f8 ff ff 	sethi 0xffff,gr0
 598:	80 40 ff e0 	addi gr15,-32,gr0
 59c:	80 fc ff e0 	setlos 0xf*ffffffe0,gr0
 5a0:	80 f4 ff e0 	setlo 0xffe0,gr0
 5a4:	80 f8 ff ff 	sethi 0xffff,gr0
 5a8:	80 f4 00 14 	setlo 0x14,gr0
 5ac:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

0000462c <D4>:
    462c:	00 00 00 04 	add\.p gr0,gr4,gr0
			462c: R_FRV_32	\.data

00004630 <PD0>:
    4630:	00 00 00 04 	add\.p gr0,gr4,gr0
			4630: R_FRV_FUNCDESC	\.text
    4634:	00 00 00 04 	add\.p gr0,gr4,gr0
			4634: R_FRV_32	\.text
Disassembly of section \.got:

00004638 <_GLOBAL_OFFSET_TABLE_-0x18>:
    4638:	00 00 00 04 	add\.p gr0,gr4,gr0
			4638: R_FRV_FUNCDESC_VALUE	\.text
    463c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4640:	00 00 00 04 	add\.p gr0,gr4,gr0
			4640: R_FRV_FUNCDESC_VALUE	\.text
    4644:	00 00 00 00 	add\.p gr0,gr0,gr0
    4648:	00 00 00 04 	add\.p gr0,gr4,gr0
			4648: R_FRV_FUNCDESC_VALUE	\.text
    464c:	00 00 00 00 	add\.p gr0,gr0,gr0

00004650 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    465c:	00 00 00 04 	add\.p gr0,gr4,gr0
			465c: R_FRV_FUNCDESC	\.text
    4660:	00 00 00 04 	add\.p gr0,gr4,gr0
			4660: R_FRV_32	\.text
    4664:	00 00 00 04 	add\.p gr0,gr4,gr0
			4664: R_FRV_32	\.data
    4668:	00 00 00 04 	add\.p gr0,gr4,gr0
			4668: R_FRV_FUNCDESC	\.text
    466c:	00 00 00 04 	add\.p gr0,gr4,gr0
			466c: R_FRV_32	\.text
    4670:	00 00 00 04 	add\.p gr0,gr4,gr0
			4670: R_FRV_32	\.text
    4674:	00 00 00 04 	add\.p gr0,gr4,gr0
			4674: R_FRV_FUNCDESC	\.text
