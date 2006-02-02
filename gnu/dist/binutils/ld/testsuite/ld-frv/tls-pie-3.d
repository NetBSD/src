#name: FRV TLS undefweak relocs, pie linking
#source: tls-3.s
#objdump: -DR -j .text -j .got -j .plt
#ld: -pie

.*:     file format elf.*frv.*

Disassembly of section \.text:

00000324 <_start>:
 324:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 328:	00 88 00 00 	nop\.p
 32c:	80 88 00 00 	nop
 330:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 334:	80 88 00 00 	nop
 338:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
 33c:	80 88 00 00 	nop
 340:	80 88 00 00 	nop
 344:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 348:	00 88 00 00 	nop\.p
 34c:	80 88 00 00 	nop
 350:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
Disassembly of section \.got:

000043d8 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			43e4: R_FRV_TLSOFF	u
