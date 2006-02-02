#name: FRV uClinux PIC relocs to local symbols, shared linking
#source: fdpic1.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -shared

.*:     file format elf.*frv.*

Disassembly of section \.text:

0000033c <F1>:
 33c:	80 3c 00 01 	call 340 <\.F0>

00000340 <\.F0>:
 340:	80 40 f0 0c 	addi gr15,12,gr0
 344:	80 fc 00 0c 	setlos 0xc,gr0
 348:	80 f4 00 0c 	setlo 0xc,gr0
 34c:	80 f8 00 00 	sethi hi\(0x0\),gr0
 350:	80 40 f0 10 	addi gr15,16,gr0
 354:	80 fc 00 10 	setlos 0x10,gr0
 358:	80 f4 00 10 	setlo 0x10,gr0
 35c:	80 f8 00 00 	sethi hi\(0x0\),gr0
 360:	80 40 ff f8 	addi gr15,-8,gr0
 364:	80 fc ff f8 	setlos 0xf*fffffff8,gr0
 368:	80 f4 ff f8 	setlo 0xfff8,gr0
 36c:	80 f8 ff ff 	sethi 0xffff,gr0
 370:	80 40 ff f0 	addi gr15,-16,gr0
 374:	80 fc ff f0 	setlos 0xf*fffffff0,gr0
 378:	80 f4 ff f0 	setlo 0xfff0,gr0
 37c:	80 f8 ff ff 	sethi 0xffff,gr0
 380:	80 f4 00 14 	setlo 0x14,gr0
 384:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

00004404 <D1>:
    4404:	00 00 00 04 	add\.p gr0,gr4,gr0
			4404: R_FRV_32	\.data

00004408 <\.D0>:
    4408:	00 00 00 00 	add\.p gr0,gr0,gr0
			4408: R_FRV_32	\.got
    440c:	00 00 00 04 	add\.p gr0,gr4,gr0
			440c: R_FRV_32	\.text
Disassembly of section \.got:

00004410 <_GLOBAL_OFFSET_TABLE_-0x8>:
    4410:	00 00 00 04 	add\.p gr0,gr4,gr0
			4410: R_FRV_FUNCDESC_VALUE	\.text
    4414:	00 00 00 00 	add\.p gr0,gr0,gr0

00004418 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    4424:	00 00 00 04 	add\.p gr0,gr4,gr0
			4424: R_FRV_32	\.text
    4428:	00 00 00 00 	add\.p gr0,gr0,gr0
			4428: R_FRV_32	\.got
    442c:	00 00 00 04 	add\.p gr0,gr4,gr0
			442c: R_FRV_32	\.data
