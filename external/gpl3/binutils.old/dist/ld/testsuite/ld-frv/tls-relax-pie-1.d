#name: FRV TLS relocs, pie linking with relaxation
#source: tls-1.s
#objdump: -DR -j .text -j .got -j .plt
#ld: -pie tmpdir/tls-1-dep.so --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

[0-9a-f ]+<_start>:
[0-9a-f ]+:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc f8 10 	setlos 0xf*fffff810,gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc f8 10 	setlos 0xf*fffff810,gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	12 fc f8 10 	setlos\.p 0xf*fffff810,gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc f8 20 	setlos 0xf*fffff820,gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc f8 20 	setlos 0xf*fffff820,gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	12 fc f8 20 	setlos\.p 0xf*fffff820,gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc 00 00 	setlos lo\(0x0\),gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc 00 00 	setlos lo\(0x0\),gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	12 fc 00 00 	setlos\.p lo\(0x0\),gr9
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	90 fc f8 20 	setlos 0xf*fffff820,gr8
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	92 fc f8 10 	setlos 0xf*fffff810,gr9
[0-9a-f ]+:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
[0-9a-f ]+:	92 fc f8 10 	setlos 0xf*fffff810,gr9
[0-9a-f ]+:	92 fc f8 20 	setlos 0xf*fffff820,gr9
[0-9a-f ]+:	92 fc 00 00 	setlos lo\(0x0\),gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc f8 10 	setlos 0xf*fffff810,gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc f8 20 	setlos 0xf*fffff820,gr9
[0-9a-f ]+:	00 88 00 00 	nop\.p
[0-9a-f ]+:	80 88 00 00 	nop
[0-9a-f ]+:	92 fc 00 00 	setlos lo\(0x0\),gr9
Disassembly of section \.got:

[0-9a-f ]+<(__data_start|_GLOBAL_OFFSET_TABLE_)>:
	\.\.\.
[0-9a-f	 ]+: R_FRV_TLSOFF	x
