#name: FRV TLS relocs, dynamic linking
#source: tls-1.s
#objdump: -DR -j .text -j .got -j .plt
#ld: tmpdir/tls-1-dep.so

.*:     file format elf.*frv.*

Disassembly of section \.text:

000102c8 <_start>:
   102c8:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
   102cc:	00 88 00 00 	nop\.p
   102d0:	80 88 00 00 	nop
   102d4:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
   102d8:	80 88 00 00 	nop
   102dc:	12 c8 f0 10 	ldi\.p @\(gr15,16\),gr9
   102e0:	80 88 00 00 	nop
   102e4:	80 88 00 00 	nop
   102e8:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   102ec:	00 88 00 00 	nop\.p
   102f0:	80 88 00 00 	nop
   102f4:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   102f8:	80 88 00 00 	nop
   102fc:	12 fc f8 10 	setlos\.p 0xf*fffff810,gr9
   10300:	80 88 00 00 	nop
   10304:	80 88 00 00 	nop
   10308:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   1030c:	00 88 00 00 	nop\.p
   10310:	80 88 00 00 	nop
   10314:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   10318:	80 88 00 00 	nop
   1031c:	12 fc f8 20 	setlos\.p 0xf*fffff820,gr9
   10320:	80 88 00 00 	nop
   10324:	80 88 00 00 	nop
   10328:	92 fc 00 00 	setlos lo\(0x0\),gr9
   1032c:	00 88 00 00 	nop\.p
   10330:	80 88 00 00 	nop
   10334:	92 fc 00 00 	setlos lo\(0x0\),gr9
   10338:	80 88 00 00 	nop
   1033c:	12 fc 00 00 	setlos\.p lo\(0x0\),gr9
   10340:	80 88 00 00 	nop
   10344:	80 88 00 00 	nop
   10348:	00 88 00 00 	nop\.p
   1034c:	90 fc f8 20 	setlos 0xf*fffff820,gr8
   10350:	00 88 00 00 	nop\.p
   10354:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   10358:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
   1035c:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   10360:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   10364:	92 fc 00 00 	setlos lo\(0x0\),gr9
   10368:	00 88 00 00 	nop\.p
   1036c:	80 88 00 00 	nop
   10370:	92 c8 f0 10 	ldi @\(gr15,16\),gr9
   10374:	00 88 00 00 	nop\.p
   10378:	80 88 00 00 	nop
   1037c:	92 fc f8 10 	setlos 0xf*fffff810,gr9
   10380:	00 88 00 00 	nop\.p
   10384:	80 88 00 00 	nop
   10388:	92 fc f8 20 	setlos 0xf*fffff820,gr9
   1038c:	00 88 00 00 	nop\.p
   10390:	80 88 00 00 	nop
   10394:	92 fc 00 00 	setlos lo\(0x0\),gr9
Disassembly of section \.got:

00014428 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
   14434:	ff ff f8 20 	cop2 -32,cpr63,cpr32,cpr63
   14438:	00 00 00 00 	add\.p gr0,gr0,gr0
			14438: R_FRV_TLSOFF	x
   1443c:	ff ff f8 10 	cop2 -32,cpr63,cpr16,cpr63
   14440:	00 00 00 00 	add\.p gr0,gr0,gr0
