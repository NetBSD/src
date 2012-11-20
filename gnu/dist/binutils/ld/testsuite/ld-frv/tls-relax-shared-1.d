#name: FRV TLS relocs, shared linking with relaxation
#source: tls-1.s
#objdump: -DR -j .text -j .got -j .plt
#ld: -shared tmpdir/tls-1-dep.so --version-script tls-1-shared.lds --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

00000354 <_start>:
 354:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
 358:	00 88 00 00 	nop\.p
 35c:	80 88 00 00 	nop
 360:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
 364:	80 88 00 00 	nop
 368:	12 c8 f0 10 	ldi\.p @\(gr15,16\),gr9
 36c:	80 88 00 00 	nop
 370:	80 88 00 00 	nop
 374:	92 c8 f0 14 	ldi @\(gr15,20\),gr9
 378:	00 88 00 00 	nop\.p
 37c:	80 88 00 00 	nop
 380:	92 c8 f0 14 	ldi @\(gr15,20\),gr9
 384:	80 88 00 00 	nop
 388:	12 c8 f0 14 	ldi\.p @\(gr15,20\),gr9
 38c:	80 88 00 00 	nop
 390:	80 88 00 00 	nop
 394:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 398:	00 88 00 00 	nop\.p
 39c:	80 88 00 00 	nop
 3a0:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 3a4:	80 88 00 00 	nop
 3a8:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
 3ac:	80 88 00 00 	nop
 3b0:	80 88 00 00 	nop
 3b4:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
 3b8:	00 88 00 00 	nop\.p
 3bc:	80 88 00 00 	nop
 3c0:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
 3c4:	80 88 00 00 	nop
 3c8:	12 c8 f0 18 	ldi\.p @\(gr15,24\),gr9
 3cc:	80 88 00 00 	nop
 3d0:	80 88 00 00 	nop
 3d4:	00 88 00 00 	nop\.p
 3d8:	90 fc f8 20 	setlos 0xf*fffff820,gr8
 3dc:	00 88 00 00 	nop\.p
 3e0:	92 fc f8 10 	setlos 0xf*fffff810,gr9
 3e4:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
 3e8:	92 c8 f0 14 	ldi @\(gr15,20\),gr9
 3ec:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 3f0:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
 3f4:	00 88 00 00 	nop\.p
 3f8:	80 88 00 00 	nop
 3fc:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
 400:	00 88 00 00 	nop\.p
 404:	80 88 00 00 	nop
 408:	92 c8 f0 14 	ldi @\(gr15,20\),gr9
 40c:	00 88 00 00 	nop\.p
 410:	80 88 00 00 	nop
 414:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 418:	00 88 00 00 	nop\.p
 41c:	80 88 00 00 	nop
 420:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
Disassembly of section \.got:

000044b8 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    44c4:	00 00 00 10 	add\.p gr0,gr16,gr0
			44c4: R_FRV_TLSOFF	\.tbss
	\.\.\.
			44c8: R_FRV_TLSOFF	x
			44cc: R_FRV_TLSOFF	\.tbss
    44d0:	00 00 07 f0 	\*unknown\*
			44d0: R_FRV_TLSOFF	\.tbss
