#name: FRV uClinux PIC relocs to (mostly) global symbols with addends, shared linking
#source: fdpic8.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -shared --version-script fdpic8min.ldv

.*:     file format elf.*frv.*

Disassembly of section \.text:

00000434 <F8>:
 434:	80 3c 00 02 	call 43c <GF1\+0x4>

00000438 <GF1>:
 438:	80 40 f0 10 	addi gr15,16,gr0
 43c:	80 fc 00 14 	setlos 0x14,gr0
 440:	80 f4 00 24 	setlo 0x24,gr0
 444:	80 f8 00 00 	sethi hi\(0x0\),gr0
 448:	80 40 f0 0c 	addi gr15,12,gr0
 44c:	80 fc 00 1c 	setlos 0x1c,gr0
 450:	80 f4 00 18 	setlo 0x18,gr0
 454:	80 f8 00 00 	sethi hi\(0x0\),gr0
 458:	80 40 ff f8 	addi gr15,-8,gr0
 45c:	80 fc ff f0 	setlos 0xf*fffffff0,gr0
 460:	80 f4 ff c8 	setlo 0xffc8,gr0
 464:	80 f8 ff ff 	sethi 0xffff,gr0
 468:	80 40 ff c4 	addi gr15,-60,gr0
 46c:	80 fc ff c4 	setlos 0xf*ffffffc4,gr0
 470:	80 f4 ff c4 	setlo 0xffc4,gr0
 474:	80 f8 ff ff 	sethi 0xffff,gr0
 478:	80 f4 00 20 	setlo 0x20,gr0
 47c:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

000044fc <D8>:
    44fc:	00 00 00 04 	add\.p gr0,gr4,gr0
			44fc: R_FRV_32	GD0

00004500 <GD0>:
    4500:	00 00 00 10 	add\.p gr0,gr16,gr0
			4500: R_FRV_32	\.got
    4504:	00 00 00 08 	add\.p gr0,gr8,gr0
			4504: R_FRV_32	\.text
Disassembly of section \.got:

00004508 <_GLOBAL_OFFSET_TABLE_-0x38>:
    4508:	00 00 00 08 	add\.p gr0,gr8,gr0
			4508: R_FRV_FUNCDESC_VALUE	\.text
    450c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4510:	00 00 00 08 	add\.p gr0,gr8,gr0
			4510: R_FRV_FUNCDESC_VALUE	\.text
    4514:	00 00 00 00 	add\.p gr0,gr0,gr0
    4518:	00 00 00 08 	add\.p gr0,gr8,gr0
			4518: R_FRV_FUNCDESC_VALUE	\.text
    451c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4520:	00 00 00 08 	add\.p gr0,gr8,gr0
			4520: R_FRV_FUNCDESC_VALUE	\.text
    4524:	00 00 00 00 	add\.p gr0,gr0,gr0
    4528:	00 00 00 08 	add\.p gr0,gr8,gr0
			4528: R_FRV_FUNCDESC_VALUE	\.text
    452c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4530:	00 00 00 08 	add\.p gr0,gr8,gr0
			4530: R_FRV_FUNCDESC_VALUE	\.text
    4534:	00 00 00 00 	add\.p gr0,gr0,gr0
    4538:	00 00 00 08 	add\.p gr0,gr8,gr0
			4538: R_FRV_FUNCDESC_VALUE	\.text
    453c:	00 00 00 00 	add\.p gr0,gr0,gr0

00004540 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    454c:	00 00 00 08 	add\.p gr0,gr8,gr0
			454c: R_FRV_32	\.got
    4550:	00 00 00 04 	add\.p gr0,gr4,gr0
			4550: R_FRV_32	GF1
    4554:	00 00 00 04 	add\.p gr0,gr4,gr0
			4554: R_FRV_32	GF2
    4558:	00 00 00 20 	add\.p gr0,gr32,gr0
			4558: R_FRV_32	\.got
    455c:	00 00 00 18 	add\.p gr0,gr24,gr0
			455c: R_FRV_32	\.got
    4560:	00 00 00 04 	add\.p gr0,gr4,gr0
			4560: R_FRV_32	GD4
    4564:	00 00 00 04 	add\.p gr0,gr4,gr0
			4564: R_FRV_32	GF3
