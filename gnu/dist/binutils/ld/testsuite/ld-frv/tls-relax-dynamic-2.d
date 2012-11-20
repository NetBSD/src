#name: FRV TLS relocs with addends, dynamic linking, relaxing
#source: tls-2.s
#objdump: -DR -j .text -j .got -j .plt
#ld: tmpdir/tls-1-dep.so --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

00010308 <_start>:
   10308:	92 fc f8 11 	setlos 0xf*fffff811,gr9
   1030c:	92 fc 08 11 	setlos 0x811,gr9
   10310:	92 c8 f0 2c 	ldi @\(gr15,44\),gr9
   10314:	00 88 00 00 	nop\.p
   10318:	80 88 00 00 	nop
   1031c:	92 fc f8 12 	setlos 0xf*fffff812,gr9
   10320:	80 88 00 00 	nop
   10324:	00 88 00 00 	nop\.p
   10328:	80 88 00 00 	nop
   1032c:	92 fc 08 12 	setlos 0x812,gr9
   10330:	80 88 00 00 	nop
   10334:	00 88 00 00 	nop\.p
   10338:	80 88 00 00 	nop
   1033c:	92 f8 00 00 	sethi hi\(0x0\),gr9
   10340:	92 f4 f8 12 	setlo 0xf812,gr9
   10344:	12 fc f8 13 	setlos\.p 0xf*fffff813,gr9
   10348:	80 88 00 00 	nop
   1034c:	80 88 00 00 	nop
   10350:	12 fc 08 13 	setlos\.p 0x813,gr9
   10354:	80 88 00 00 	nop
   10358:	80 88 00 00 	nop
   1035c:	12 f8 00 00 	sethi\.p hi\(0x0\),gr9
   10360:	80 88 00 00 	nop
   10364:	92 f4 f8 13 	setlo 0xf813,gr9
   10368:	80 88 00 00 	nop
   1036c:	92 fc f8 14 	setlos 0xf*fffff814,gr9
   10370:	80 88 00 00 	nop
   10374:	92 fc 08 14 	setlos 0x814,gr9
   10378:	92 f8 00 00 	sethi hi\(0x0\),gr9
   1037c:	92 f4 f8 14 	setlo 0xf814,gr9
   10380:	92 fc f8 21 	setlos 0xf*fffff821,gr9
   10384:	92 fc 08 21 	setlos 0x821,gr9
   10388:	92 c8 f0 14 	ldi @\(gr15,20\),gr9
   1038c:	00 88 00 00 	nop\.p
   10390:	80 88 00 00 	nop
   10394:	92 fc f8 22 	setlos 0xf*fffff822,gr9
   10398:	80 88 00 00 	nop
   1039c:	00 88 00 00 	nop\.p
   103a0:	80 88 00 00 	nop
   103a4:	92 fc 08 22 	setlos 0x822,gr9
   103a8:	80 88 00 00 	nop
   103ac:	00 88 00 00 	nop\.p
   103b0:	80 88 00 00 	nop
   103b4:	92 f8 00 00 	sethi hi\(0x0\),gr9
   103b8:	92 f4 f8 22 	setlo 0xf822,gr9
   103bc:	12 fc f8 23 	setlos\.p 0xf*fffff823,gr9
   103c0:	80 88 00 00 	nop
   103c4:	80 88 00 00 	nop
   103c8:	12 fc 08 23 	setlos\.p 0x823,gr9
   103cc:	80 88 00 00 	nop
   103d0:	80 88 00 00 	nop
   103d4:	12 f8 00 00 	sethi\.p hi\(0x0\),gr9
   103d8:	80 88 00 00 	nop
   103dc:	92 f4 f8 23 	setlo 0xf823,gr9
   103e0:	80 88 00 00 	nop
   103e4:	92 fc f8 24 	setlos 0xf*fffff824,gr9
   103e8:	80 88 00 00 	nop
   103ec:	92 fc 08 24 	setlos 0x824,gr9
   103f0:	92 f8 00 00 	sethi hi\(0x0\),gr9
   103f4:	92 f4 f8 24 	setlo 0xf824,gr9
   103f8:	92 fc 00 01 	setlos 0x1,gr9
   103fc:	92 fc 10 01 	setlos 0x1001,gr9
   10400:	92 c8 f0 24 	ldi @\(gr15,36\),gr9
   10404:	00 88 00 00 	nop\.p
   10408:	80 88 00 00 	nop
   1040c:	92 fc 00 02 	setlos 0x2,gr9
   10410:	80 88 00 00 	nop
   10414:	00 88 00 00 	nop\.p
   10418:	80 88 00 00 	nop
   1041c:	92 fc 10 02 	setlos 0x1002,gr9
   10420:	80 88 00 00 	nop
   10424:	00 88 00 00 	nop\.p
   10428:	80 88 00 00 	nop
   1042c:	92 f8 00 01 	sethi 0x1,gr9
   10430:	92 f4 00 02 	setlo 0x2,gr9
   10434:	12 fc 00 03 	setlos\.p 0x3,gr9
   10438:	80 88 00 00 	nop
   1043c:	80 88 00 00 	nop
   10440:	12 fc 10 03 	setlos\.p 0x1003,gr9
   10444:	80 88 00 00 	nop
   10448:	80 88 00 00 	nop
   1044c:	12 f8 00 01 	sethi\.p 0x1,gr9
   10450:	80 88 00 00 	nop
   10454:	92 f4 00 03 	setlo 0x3,gr9
   10458:	80 88 00 00 	nop
   1045c:	92 fc 00 04 	setlos 0x4,gr9
   10460:	80 88 00 00 	nop
   10464:	92 fc 10 04 	setlos 0x1004,gr9
   10468:	92 f8 00 01 	sethi 0x1,gr9
   1046c:	92 f4 00 04 	setlo 0x4,gr9
   10470:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
   10474:	92 c8 f0 1c 	ldi @\(gr15,28\),gr9
   10478:	92 c8 f0 28 	ldi @\(gr15,40\),gr9
   1047c:	00 88 00 00 	nop\.p
   10480:	80 88 00 00 	nop
   10484:	92 c8 f0 38 	ldi @\(gr15,56\),gr9
   10488:	80 88 00 00 	nop
   1048c:	00 88 00 00 	nop\.p
   10490:	80 88 00 00 	nop
   10494:	92 c8 f0 34 	ldi @\(gr15,52\),gr9
   10498:	80 88 00 00 	nop
   1049c:	00 88 00 00 	nop\.p
   104a0:	80 88 00 00 	nop
   104a4:	92 c8 f0 30 	ldi @\(gr15,48\),gr9
   104a8:	80 88 00 00 	nop
   104ac:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
   104b0:	80 88 00 00 	nop
   104b4:	80 88 00 00 	nop
   104b8:	12 c8 f0 10 	ldi\.p @\(gr15,16\),gr9
   104bc:	80 88 00 00 	nop
   104c0:	80 88 00 00 	nop
   104c4:	12 c8 f0 20 	ldi\.p @\(gr15,32\),gr9
   104c8:	80 88 00 00 	nop
   104cc:	80 88 00 00 	nop
Disassembly of section \.got:

00014568 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
   14574:	00 00 00 03 	add\.p gr0,gr3,gr0
			14574: R_FRV_TLSOFF	x
   14578:	00 00 10 03 	add\.p sp,gr3,gr0
			14578: R_FRV_TLSOFF	x
   1457c:	00 00 f8 21 	\*unknown\*
   14580:	00 00 00 01 	add\.p gr0,sp,gr0
			14580: R_FRV_TLSOFF	x
   14584:	00 00 10 01 	add\.p sp,sp,gr0
			14584: R_FRV_TLSOFF	x
   14588:	00 01 00 03 	add\.p gr16,gr3,gr0
			14588: R_FRV_TLSOFF	x
   1458c:	00 01 00 01 	add\.p gr16,sp,gr0
   14590:	00 01 00 01 	add\.p gr16,sp,gr0
			14590: R_FRV_TLSOFF	x
   14594:	00 00 f8 11 	\*unknown\*
   14598:	00 01 00 02 	add\.p gr16,fp,gr0
			14598: R_FRV_TLSOFF	x
   1459c:	00 00 10 02 	add\.p sp,fp,gr0
			1459c: R_FRV_TLSOFF	x
   145a0:	00 00 00 02 	add\.p gr0,fp,gr0
			145a0: R_FRV_TLSOFF	x
