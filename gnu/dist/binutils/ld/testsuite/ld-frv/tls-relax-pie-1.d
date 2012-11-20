#name: FRV TLS relocs, pie linking with relaxation
#source: tls-1.s
#objdump: -DR -j .text -j .got -j .plt
#ld: -pie tmpdir/tls-1-dep.so --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

000003a0 <_start>:
 3a0:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 3a4:	00 88 00 00 	nop\.p
 3a8:	80 88 00 00 	nop
 3ac:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 3b0:	80 88 00 00 	nop
 3b4:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
 3b8:	80 88 00 00 	nop
 3bc:	80 88 00 00 	nop
 3c0:	92 fc f8 10 	setlos 0xf*fffff810,gr9
 3c4:	00 88 00 00 	nop\.p
 3c8:	80 88 00 00 	nop
 3cc:	92 fc f8 10 	setlos 0xf*fffff810,gr9
 3d0:	80 88 00 00 	nop
 3d4:	12 fc f8 10 	setlos\.p 0xf*fffff810,gr9
 3d8:	80 88 00 00 	nop
 3dc:	80 88 00 00 	nop
 3e0:	92 fc f8 20 	setlos 0xf*fffff820,gr9
 3e4:	00 88 00 00 	nop\.p
 3e8:	80 88 00 00 	nop
 3ec:	92 fc f8 20 	setlos 0xf*fffff820,gr9
 3f0:	80 88 00 00 	nop
 3f4:	12 fc f8 20 	setlos\.p 0xf*fffff820,gr9
 3f8:	80 88 00 00 	nop
 3fc:	80 88 00 00 	nop
 400:	92 fc 00 00 	setlos lo\(0x0\),gr9
 404:	00 88 00 00 	nop\.p
 408:	80 88 00 00 	nop
 40c:	92 fc 00 00 	setlos lo\(0x0\),gr9
 410:	80 88 00 00 	nop
 414:	12 fc 00 00 	setlos\.p lo\(0x0\),gr9
 418:	80 88 00 00 	nop
 41c:	80 88 00 00 	nop
 420:	00 88 00 00 	nop\.p
 424:	90 fc f8 20 	setlos 0xf*fffff820,gr8
 428:	00 88 00 00 	nop\.p
 42c:	92 fc f8 10 	setlos 0xf*fffff810,gr9
 430:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 434:	92 fc f8 10 	setlos 0xf*fffff810,gr9
 438:	92 fc f8 20 	setlos 0xf*fffff820,gr9
 43c:	92 fc 00 00 	setlos lo\(0x0\),gr9
 440:	00 88 00 00 	nop\.p
 444:	80 88 00 00 	nop
 448:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 44c:	00 88 00 00 	nop\.p
 450:	80 88 00 00 	nop
 454:	92 fc f8 10 	setlos 0xf*fffff810,gr9
 458:	00 88 00 00 	nop\.p
 45c:	80 88 00 00 	nop
 460:	92 fc f8 20 	setlos 0xf*fffff820,gr9
 464:	00 88 00 00 	nop\.p
 468:	80 88 00 00 	nop
 46c:	92 fc 00 00 	setlos lo\(0x0\),gr9
Disassembly of section \.got:

00004508 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			4514: R_FRV_TLSOFF	x
