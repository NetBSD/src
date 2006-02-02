#name: FRV uClinux PIC relocs to hidden symbols, shared linking
#source: fdpic3.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -shared

.*:     file format elf.*frv.*

Disassembly of section \.text:

0000038c <F3>:
 38c:	80 3c 00 01 	call 390 <HF0>

00000390 <HF0>:
 390:	80 40 f0 0c 	addi gr15,12,gr0
 394:	80 fc 00 18 	setlos 0x18,gr0
 398:	80 f4 00 1c 	setlo 0x1c,gr0
 39c:	80 f8 00 00 	sethi hi\(0x0\),gr0
 3a0:	80 40 f0 10 	addi gr15,16,gr0
 3a4:	80 fc 00 20 	setlos 0x20,gr0
 3a8:	80 f4 00 14 	setlo 0x14,gr0
 3ac:	80 f8 00 00 	sethi hi\(0x0\),gr0
 3b0:	80 40 ff f8 	addi gr15,-8,gr0
 3b4:	80 fc ff e8 	setlos 0xf*ffffffe8,gr0
 3b8:	80 f4 ff d8 	setlo 0xffd8,gr0
 3bc:	80 f8 ff ff 	sethi 0xffff,gr0
 3c0:	80 40 ff c0 	addi gr15,-64,gr0
 3c4:	80 fc ff c0 	setlos 0xf*ffffffc0,gr0
 3c8:	80 f4 ff c0 	setlo 0xffc0,gr0
 3cc:	80 f8 ff ff 	sethi 0xffff,gr0
 3d0:	80 f4 00 24 	setlo 0x24,gr0
 3d4:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

00004454 <D3>:
    4454:	00 00 00 04 	add\.p gr0,gr4,gr0
			4454: R_FRV_32	\.data

00004458 <HD0>:
    4458:	00 00 00 08 	add\.p gr0,gr8,gr0
			4458: R_FRV_32	\.got
    445c:	00 00 00 04 	add\.p gr0,gr4,gr0
			445c: R_FRV_32	\.text
Disassembly of section \.got:

00004460 <_GLOBAL_OFFSET_TABLE_-0x38>:
    4460:	00 00 00 04 	add\.p gr0,gr4,gr0
			4460: R_FRV_FUNCDESC_VALUE	\.text
    4464:	00 00 00 00 	add\.p gr0,gr0,gr0
    4468:	00 00 00 04 	add\.p gr0,gr4,gr0
			4468: R_FRV_FUNCDESC_VALUE	\.text
    446c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4470:	00 00 00 04 	add\.p gr0,gr4,gr0
			4470: R_FRV_FUNCDESC_VALUE	\.text
    4474:	00 00 00 00 	add\.p gr0,gr0,gr0
    4478:	00 00 00 04 	add\.p gr0,gr4,gr0
			4478: R_FRV_FUNCDESC_VALUE	\.text
    447c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4480:	00 00 00 04 	add\.p gr0,gr4,gr0
			4480: R_FRV_FUNCDESC_VALUE	\.text
    4484:	00 00 00 00 	add\.p gr0,gr0,gr0
    4488:	00 00 00 04 	add\.p gr0,gr4,gr0
			4488: R_FRV_FUNCDESC_VALUE	\.text
    448c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4490:	00 00 00 04 	add\.p gr0,gr4,gr0
			4490: R_FRV_FUNCDESC_VALUE	\.text
    4494:	00 00 00 00 	add\.p gr0,gr0,gr0

00004498 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    44a4:	00 00 00 04 	add\.p gr0,gr4,gr0
			44a4: R_FRV_32	\.text
    44a8:	00 00 00 00 	add\.p gr0,gr0,gr0
			44a8: R_FRV_32	\.got
    44ac:	00 00 00 28 	add\.p gr0,gr40,gr0
			44ac: R_FRV_32	\.got
    44b0:	00 00 00 04 	add\.p gr0,gr4,gr0
			44b0: R_FRV_32	\.text
    44b4:	00 00 00 04 	add\.p gr0,gr4,gr0
			44b4: R_FRV_32	\.text
    44b8:	00 00 00 18 	add\.p gr0,gr24,gr0
			44b8: R_FRV_32	\.got
    44bc:	00 00 00 04 	add\.p gr0,gr4,gr0
			44bc: R_FRV_32	\.data
