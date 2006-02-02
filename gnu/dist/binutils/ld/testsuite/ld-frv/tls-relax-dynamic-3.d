#name: FRV TLS undefweak relocs, dynamic linking with relaxation
#source: tls-3.s
#objdump: -DR -j .text -j .got -j .plt
#ld: tmpdir/tls-1-dep.so --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

00010294 <_start>:
   10294:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
   10298:	00 88 00 00 	nop\.p
   1029c:	80 88 00 00 	nop
   102a0:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
   102a4:	80 88 00 00 	nop
   102a8:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
   102ac:	80 88 00 00 	nop
   102b0:	80 88 00 00 	nop
   102b4:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
   102b8:	00 88 00 00 	nop\.p
   102bc:	80 88 00 00 	nop
   102c0:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
Disassembly of section \.got:

00014350 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			1435c: R_FRV_TLSOFF	u
