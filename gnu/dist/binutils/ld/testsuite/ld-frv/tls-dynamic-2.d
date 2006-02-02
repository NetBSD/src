#name: FRV TLS relocs with addends, dynamic linking
#source: tls-2.s
#objdump: -DR -j .text -j .got -j .plt
#ld: tmpdir/tls-1-dep.so

.*:     file format elf.*frv.*

Disassembly of section \.plt:

00010308 <\.plt>:
   10308:	c0 3a 40 00 	bralr
   1030c:	92 fc 08 21 	setlos 0x821,gr9
   10310:	c0 3a 40 00 	bralr
   10314:	12 f8 00 00 	sethi\.p hi\(0x0\),gr9
   10318:	92 f4 f8 21 	setlo 0xf821,gr9
   1031c:	c0 3a 40 00 	bralr
   10320:	92 fc 00 01 	setlos 0x1,gr9
   10324:	c0 3a 40 00 	bralr
   10328:	92 c8 ff bc 	ldi @\(gr15,-68\),gr9
   1032c:	c0 3a 40 00 	bralr
   10330:	92 fc f8 11 	setlos 0xf*fffff811,gr9
   10334:	c0 3a 40 00 	bralr
   10338:	92 fc 10 01 	setlos 0x1001,gr9
   1033c:	c0 3a 40 00 	bralr
   10340:	92 c8 ff d4 	ldi @\(gr15,-44\),gr9
   10344:	c0 3a 40 00 	bralr
   10348:	92 fc 08 11 	setlos 0x811,gr9
   1034c:	c0 3a 40 00 	bralr
   10350:	12 f8 00 01 	sethi\.p 0x1,gr9
   10354:	92 f4 00 01 	setlo 0x1,gr9
   10358:	c0 3a 40 00 	bralr
   1035c:	92 c8 ff ec 	ldi @\(gr15,-20\),gr9
   10360:	c0 3a 40 00 	bralr
   10364:	12 f8 00 00 	sethi\.p hi\(0x0\),gr9
   10368:	92 f4 f8 11 	setlo 0xf811,gr9
   1036c:	c0 3a 40 00 	bralr
   10370:	92 fc f8 21 	setlos 0xf*fffff821,gr9
   10374:	c0 3a 40 00 	bralr
Disassembly of section \.text:

00010378 <_start>:
   10378:	92 fc f8 11 	setlos 0xf*fffff811,gr9
   1037c:	92 fc 08 11 	setlos 0x811,gr9
   10380:	92 c8 ff f4 	ldi @\(gr15,-12\),gr9
   10384:	00 88 00 00 	nop\.p
   10388:	80 88 00 00 	nop
   1038c:	92 fc f8 12 	setlos 0xf*fffff812,gr9
   10390:	80 88 00 00 	nop
   10394:	00 88 00 00 	nop\.p
   10398:	80 88 00 00 	nop
   1039c:	92 fc 08 12 	setlos 0x812,gr9
   103a0:	80 88 00 00 	nop
   103a4:	00 88 00 00 	nop\.p
   103a8:	80 88 00 00 	nop
   103ac:	92 f8 00 00 	sethi hi\(0x0\),gr9
   103b0:	92 f4 f8 12 	setlo 0xf812,gr9
   103b4:	12 fc f8 13 	setlos\.p 0xf*fffff813,gr9
   103b8:	80 88 00 00 	nop
   103bc:	80 88 00 00 	nop
   103c0:	12 fc 08 13 	setlos\.p 0x813,gr9
   103c4:	80 88 00 00 	nop
   103c8:	80 88 00 00 	nop
   103cc:	12 f8 00 00 	sethi\.p hi\(0x0\),gr9
   103d0:	80 88 00 00 	nop
   103d4:	92 f4 f8 13 	setlo 0xf813,gr9
   103d8:	80 88 00 00 	nop
   103dc:	92 fc f8 14 	setlos 0xf*fffff814,gr9
   103e0:	80 88 00 00 	nop
   103e4:	92 fc 08 14 	setlos 0x814,gr9
   103e8:	92 f8 00 00 	sethi hi\(0x0\),gr9
   103ec:	92 f4 f8 14 	setlo 0xf814,gr9
   103f0:	92 fc f8 21 	setlos 0xf*fffff821,gr9
   103f4:	92 fc 08 21 	setlos 0x821,gr9
   103f8:	92 c8 ff ac 	ldi @\(gr15,-84\),gr9
   103fc:	00 88 00 00 	nop\.p
   10400:	80 88 00 00 	nop
   10404:	92 fc f8 22 	setlos 0xf*fffff822,gr9
   10408:	80 88 00 00 	nop
   1040c:	00 88 00 00 	nop\.p
   10410:	80 88 00 00 	nop
   10414:	92 fc 08 22 	setlos 0x822,gr9
   10418:	80 88 00 00 	nop
   1041c:	00 88 00 00 	nop\.p
   10420:	80 88 00 00 	nop
   10424:	92 f8 00 00 	sethi hi\(0x0\),gr9
   10428:	92 f4 f8 22 	setlo 0xf822,gr9
   1042c:	12 fc f8 23 	setlos\.p 0xf*fffff823,gr9
   10430:	80 88 00 00 	nop
   10434:	80 88 00 00 	nop
   10438:	12 fc 08 23 	setlos\.p 0x823,gr9
   1043c:	80 88 00 00 	nop
   10440:	80 88 00 00 	nop
   10444:	12 f8 00 00 	sethi\.p hi\(0x0\),gr9
   10448:	80 88 00 00 	nop
   1044c:	92 f4 f8 23 	setlo 0xf823,gr9
   10450:	80 88 00 00 	nop
   10454:	92 fc f8 24 	setlos 0xf*fffff824,gr9
   10458:	80 88 00 00 	nop
   1045c:	92 fc 08 24 	setlos 0x824,gr9
   10460:	92 f8 00 00 	sethi hi\(0x0\),gr9
   10464:	92 f4 f8 24 	setlo 0xf824,gr9
   10468:	92 fc 00 01 	setlos 0x1,gr9
   1046c:	92 fc 10 01 	setlos 0x1001,gr9
   10470:	92 c8 ff e4 	ldi @\(gr15,-28\),gr9
   10474:	00 88 00 00 	nop\.p
   10478:	80 88 00 00 	nop
   1047c:	92 fc 00 02 	setlos 0x2,gr9
   10480:	80 88 00 00 	nop
   10484:	00 88 00 00 	nop\.p
   10488:	80 88 00 00 	nop
   1048c:	92 fc 10 02 	setlos 0x1002,gr9
   10490:	80 88 00 00 	nop
   10494:	00 88 00 00 	nop\.p
   10498:	80 88 00 00 	nop
   1049c:	92 f8 00 01 	sethi 0x1,gr9
   104a0:	92 f4 00 02 	setlo 0x2,gr9
   104a4:	12 fc 00 03 	setlos\.p 0x3,gr9
   104a8:	80 88 00 00 	nop
   104ac:	80 88 00 00 	nop
   104b0:	12 fc 10 03 	setlos\.p 0x1003,gr9
   104b4:	80 88 00 00 	nop
   104b8:	80 88 00 00 	nop
   104bc:	12 f8 00 01 	sethi\.p 0x1,gr9
   104c0:	80 88 00 00 	nop
   104c4:	92 f4 00 03 	setlo 0x3,gr9
   104c8:	80 88 00 00 	nop
   104cc:	92 fc 00 04 	setlos 0x4,gr9
   104d0:	80 88 00 00 	nop
   104d4:	92 fc 10 04 	setlos 0x1004,gr9
   104d8:	92 f8 00 01 	sethi 0x1,gr9
   104dc:	92 f4 00 04 	setlo 0x4,gr9
   104e0:	92 c8 ff bc 	ldi @\(gr15,-68\),gr9
   104e4:	92 c8 ff d4 	ldi @\(gr15,-44\),gr9
   104e8:	92 c8 ff ec 	ldi @\(gr15,-20\),gr9
   104ec:	00 88 00 00 	nop\.p
   104f0:	80 88 00 00 	nop
   104f4:	92 c8 f0 20 	ldi @\(gr15,32\),gr9
   104f8:	80 88 00 00 	nop
   104fc:	00 88 00 00 	nop\.p
   10500:	80 88 00 00 	nop
   10504:	92 c8 f0 1c 	ldi @\(gr15,28\),gr9
   10508:	80 88 00 00 	nop
   1050c:	00 88 00 00 	nop\.p
   10510:	80 88 00 00 	nop
   10514:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
   10518:	80 88 00 00 	nop
   1051c:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
   10520:	80 88 00 00 	nop
   10524:	80 88 00 00 	nop
   10528:	12 c8 f0 10 	ldi\.p @\(gr15,16\),gr9
   1052c:	80 88 00 00 	nop
   10530:	80 88 00 00 	nop
   10534:	12 c8 f0 14 	ldi\.p @\(gr15,20\),gr9
   10538:	80 88 00 00 	nop
   1053c:	80 88 00 00 	nop
Disassembly of section \.got:

000145f8 <_GLOBAL_OFFSET_TABLE_-0x60>:
   145f8:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   145fc:	00 00 08 21 	\*unknown\*
   14600:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   14604:	00 00 f8 21 	\*unknown\*
   14608:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   1460c:	00 00 00 01 	add\.p gr0,sp,gr0
   14610:	00 00 00 00 	add\.p gr0,gr0,gr0
			14610: R_FRV_TLSDESC_VALUE	x
   14614:	00 00 00 01 	add\.p gr0,sp,gr0
   14618:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   1461c:	ff ff f8 11 	cop2 -32,cpr63,cpr17,cpr63
   14620:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   14624:	00 00 10 01 	add\.p sp,sp,gr0
   14628:	00 00 00 00 	add\.p gr0,gr0,gr0
			14628: R_FRV_TLSDESC_VALUE	x
   1462c:	00 00 10 01 	add\.p sp,sp,gr0
   14630:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   14634:	00 00 08 11 	\*unknown\*
   14638:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   1463c:	00 01 00 01 	add\.p gr16,sp,gr0
   14640:	00 00 00 00 	add\.p gr0,gr0,gr0
			14640: R_FRV_TLSDESC_VALUE	x
   14644:	00 01 00 01 	add\.p gr16,sp,gr0
   14648:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   1464c:	00 00 f8 11 	\*unknown\*
   14650:	00 01 03 08 	cmpb\.p gr16,gr8,icc0
   14654:	ff ff f8 21 	cop2 -32,cpr63,cpr33,cpr63

00014658 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
   14664:	00 00 00 03 	add\.p gr0,gr3,gr0
			14664: R_FRV_TLSOFF	x
   14668:	00 00 10 03 	add\.p sp,gr3,gr0
			14668: R_FRV_TLSOFF	x
   1466c:	00 01 00 03 	add\.p gr16,gr3,gr0
			1466c: R_FRV_TLSOFF	x
   14670:	00 01 00 02 	add\.p gr16,fp,gr0
			14670: R_FRV_TLSOFF	x
   14674:	00 00 10 02 	add\.p sp,fp,gr0
			14674: R_FRV_TLSOFF	x
   14678:	00 00 00 02 	add\.p gr0,fp,gr0
			14678: R_FRV_TLSOFF	x
