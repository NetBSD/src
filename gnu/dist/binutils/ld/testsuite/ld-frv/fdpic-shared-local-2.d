#name: FRV uClinux PIC relocs to forced-local symbols, shared linking
#source: fdpic2.s
#objdump: -DR -j .text -j .data -j .got -j .plt
#ld: -shared --version-script fdpic2.ldv

.*:     file format elf.*frv.*

Disassembly of section \.text:

00000228 <F2>:
 228:	80 3c 00 01 	call 22c <GF0>

0000022c <GF0>:
 22c:	80 40 f0 10 	addi gr15,16,gr0
 230:	80 fc 00 24 	setlos 0x24,gr0
 234:	80 f4 00 20 	setlo 0x20,gr0
 238:	80 f8 00 00 	sethi hi\(0x0\),gr0
 23c:	80 40 f0 0c 	addi gr15,12,gr0
 240:	80 fc 00 18 	setlos 0x18,gr0
 244:	80 f4 00 14 	setlo 0x14,gr0
 248:	80 f8 00 00 	sethi hi\(0x0\),gr0
 24c:	80 40 ff f8 	addi gr15,-8,gr0
 250:	80 fc ff d0 	setlos 0xf*ffffffd0,gr0
 254:	80 f4 ff c8 	setlo 0xffc8,gr0
 258:	80 f8 ff ff 	sethi 0xffff,gr0
 25c:	80 40 ff bc 	addi gr15,-68,gr0
 260:	80 fc ff bc 	setlos 0xf*ffffffbc,gr0
 264:	80 f4 ff bc 	setlo 0xffbc,gr0
 268:	80 f8 ff ff 	sethi 0xffff,gr0
 26c:	80 f4 00 1c 	setlo 0x1c,gr0
 270:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

000042f0 <D2>:
    42f0:	00 00 00 04 	add\.p gr0,gr4,gr0
			42f0: R_FRV_32	\.data

000042f4 <GD0>:
    42f4:	00 00 00 18 	add\.p gr0,gr24,gr0
			42f4: R_FRV_32	\.got
    42f8:	00 00 00 04 	add\.p gr0,gr4,gr0
			42f8: R_FRV_32	\.text
Disassembly of section \.got:

00004300 <_GLOBAL_OFFSET_TABLE_-0x38>:
    4300:	00 00 00 04 	add\.p gr0,gr4,gr0
			4300: R_FRV_FUNCDESC_VALUE	\.text
    4304:	00 00 00 00 	add\.p gr0,gr0,gr0
    4308:	00 00 00 04 	add\.p gr0,gr4,gr0
			4308: R_FRV_FUNCDESC_VALUE	\.text
    430c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4310:	00 00 00 04 	add\.p gr0,gr4,gr0
			4310: R_FRV_FUNCDESC_VALUE	\.text
    4314:	00 00 00 00 	add\.p gr0,gr0,gr0
    4318:	00 00 00 04 	add\.p gr0,gr4,gr0
			4318: R_FRV_FUNCDESC_VALUE	\.text
    431c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4320:	00 00 00 04 	add\.p gr0,gr4,gr0
			4320: R_FRV_FUNCDESC_VALUE	\.text
    4324:	00 00 00 00 	add\.p gr0,gr0,gr0
    4328:	00 00 00 04 	add\.p gr0,gr4,gr0
			4328: R_FRV_FUNCDESC_VALUE	\.text
    432c:	00 00 00 00 	add\.p gr0,gr0,gr0
    4330:	00 00 00 04 	add\.p gr0,gr4,gr0
			4330: R_FRV_FUNCDESC_VALUE	\.text
    4334:	00 00 00 00 	add\.p gr0,gr0,gr0

00004338 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    4344:	00 00 00 10 	add\.p gr0,gr16,gr0
			4344: R_FRV_32	\.got
    4348:	00 00 00 04 	add\.p gr0,gr4,gr0
			4348: R_FRV_32	\.text
    434c:	00 00 00 28 	add\.p gr0,gr40,gr0
			434c: R_FRV_32	\.got
    4350:	00 00 00 20 	add\.p gr0,gr32,gr0
			4350: R_FRV_32	\.got
    4354:	00 00 00 04 	add\.p gr0,gr4,gr0
			4354: R_FRV_32	\.data
    4358:	00 00 00 04 	add\.p gr0,gr4,gr0
			4358: R_FRV_32	\.text
    435c:	00 00 00 04 	add\.p gr0,gr4,gr0
			435c: R_FRV_32	\.text
