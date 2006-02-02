#name: FRV TLS undefweak relocs, shared linking with relaxation
#source: tls-3.s
#objdump: -DR -j .text -j .got -j .plt
#ld: -shared --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

000002f4 <_start>:
 2f4:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 2f8:	00 88 00 00 	nop\.p
 2fc:	80 88 00 00 	nop
 300:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 304:	80 88 00 00 	nop
 308:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
 30c:	80 88 00 00 	nop
 310:	80 88 00 00 	nop
 314:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 318:	00 88 00 00 	nop\.p
 31c:	80 88 00 00 	nop
 320:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
Disassembly of section \.got:

000043a8 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			43b4: R_FRV_TLSOFF	u
