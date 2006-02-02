#name: FRV uClinux PIC relocs to undefined symbols, shared linking
#source: fdpic5.s
#objdump: -DRz -j .text -j .data -j .got -j .plt
#ld: -shared

.*:     file format elf.*frv.*

Disassembly of section \.plt:

000004a8 <\.plt>:
 4a8:	00 00 00 10 	add\.p gr0,gr16,gr0
 4ac:	c0 1a 00 06 	bra 4c4 <F5-0x10>
 4b0:	00 00 00 08 	add\.p gr0,gr8,gr0
 4b4:	c0 1a 00 04 	bra 4c4 <F5-0x10>
 4b8:	00 00 00 00 	add\.p gr0,gr0,gr0
 4bc:	c0 1a 00 02 	bra 4c4 <F5-0x10>
 4c0:	00 00 00 18 	add\.p gr0,gr24,gr0
 4c4:	88 08 f1 40 	ldd @\(gr15,gr0\),gr4
 4c8:	80 30 40 00 	jmpl @\(gr4,gr0\)
 4cc:	9c cc ff f0 	lddi @\(gr15,-16\),gr14
 4d0:	80 30 e0 00 	jmpl @\(gr14,gr0\)
Disassembly of section \.text:

000004d4 <F5>:
 4d4:	fe 3f ff fe 	call 4cc <F5-0x8>
 4d8:	80 40 f0 0c 	addi gr15,12,gr0
 4dc:	80 fc 00 24 	setlos 0x24,gr0
 4e0:	80 f4 00 20 	setlo 0x20,gr0
 4e4:	80 f8 00 00 	sethi hi\(0x0\),gr0
 4e8:	80 40 f0 10 	addi gr15,16,gr0
 4ec:	80 fc 00 1c 	setlos 0x1c,gr0
 4f0:	80 f4 00 18 	setlo 0x18,gr0
 4f4:	80 f8 00 00 	sethi hi\(0x0\),gr0
 4f8:	80 40 ff f8 	addi gr15,-8,gr0
 4fc:	80 fc ff e8 	setlos 0xf*ffffffe8,gr0
 500:	80 f4 ff e0 	setlo 0xffe0,gr0
 504:	80 f8 ff ff 	sethi 0xffff,gr0
 508:	80 f4 00 14 	setlo 0x14,gr0
 50c:	80 f8 00 00 	sethi hi\(0x0\),gr0
Disassembly of section \.data:

000045a4 <D5>:
    45a4:	00 00 00 00 	add\.p gr0,gr0,gr0
			45a4: R_FRV_32	UD0
    45a8:	00 00 00 00 	add\.p gr0,gr0,gr0
			45a8: R_FRV_FUNCDESC	UFb
    45ac:	00 00 00 00 	add\.p gr0,gr0,gr0
			45ac: R_FRV_32	UFb
Disassembly of section \.got:

000045b0 <_GLOBAL_OFFSET_TABLE_-0x20>:
    45b0:	00 00 04 c4 	addxcc\.p gr0,gr4,gr0,icc1
			45b0: R_FRV_FUNCDESC_VALUE	UF9
    45b4:	00 00 00 00 	add\.p gr0,gr0,gr0
    45b8:	00 00 04 ac 	addx\.p gr0,gr44,gr0,icc1
			45b8: R_FRV_FUNCDESC_VALUE	UF8
    45bc:	00 00 00 00 	add\.p gr0,gr0,gr0
    45c0:	00 00 04 bc 	addx\.p gr0,gr60,gr0,icc1
			45c0: R_FRV_FUNCDESC_VALUE	UF0
    45c4:	00 00 00 00 	add\.p gr0,gr0,gr0
    45c8:	00 00 04 b4 	addx\.p gr0,gr52,gr0,icc1
			45c8: R_FRV_FUNCDESC_VALUE	UF7
    45cc:	00 00 00 00 	add\.p gr0,gr0,gr0

000045d0 <_GLOBAL_OFFSET_TABLE_>:
    45d0:	00 00 00 00 	add\.p gr0,gr0,gr0
    45d4:	00 00 00 00 	add\.p gr0,gr0,gr0
    45d8:	00 00 00 00 	add\.p gr0,gr0,gr0
    45dc:	00 00 00 00 	add\.p gr0,gr0,gr0
			45dc: R_FRV_32	UF1
    45e0:	00 00 00 00 	add\.p gr0,gr0,gr0
			45e0: R_FRV_FUNCDESC	UF4
    45e4:	00 00 00 00 	add\.p gr0,gr0,gr0
			45e4: R_FRV_32	UD1
    45e8:	00 00 00 00 	add\.p gr0,gr0,gr0
			45e8: R_FRV_FUNCDESC	UF6
    45ec:	00 00 00 00 	add\.p gr0,gr0,gr0
			45ec: R_FRV_FUNCDESC	UF5
    45f0:	00 00 00 00 	add\.p gr0,gr0,gr0
			45f0: R_FRV_32	UF3
    45f4:	00 00 00 00 	add\.p gr0,gr0,gr0
			45f4: R_FRV_32	UF2
