#name: FRV TLS relocs with addends, shared linking with static TLS, relaxing
#source: tls-2.s
#as: --defsym static_tls=1
#objdump: -DR -j .text -j .got -j .plt
#ld: -shared tmpdir/tls-1-dep.so --version-script tls-1-shared.lds --relax

.*:     file format elf.*frv.*

Disassembly of section \.text:

00000454 <_start>:
 454:	92 c8 f0 34 	ldi @\(gr15,52\),gr9
 458:	92 c8 f0 44 	ldi @\(gr15,68\),gr9
 45c:	92 c8 f0 5c 	ldi @\(gr15,92\),gr9
 460:	00 88 00 00 	nop\.p
 464:	80 88 00 00 	nop
 468:	92 c8 f0 7c 	ldi @\(gr15,124\),gr9
 46c:	80 88 00 00 	nop
 470:	00 88 00 00 	nop\.p
 474:	80 88 00 00 	nop
 478:	92 c8 f0 84 	ldi @\(gr15,132\),gr9
 47c:	80 88 00 00 	nop
 480:	00 88 00 00 	nop\.p
 484:	80 88 00 00 	nop
 488:	92 c8 f0 94 	ldi @\(gr15,148\),gr9
 48c:	80 88 00 00 	nop
 490:	12 c8 f0 38 	ldi\.p @\(gr15,56\),gr9
 494:	80 88 00 00 	nop
 498:	80 88 00 00 	nop
 49c:	12 c8 f0 48 	ldi\.p @\(gr15,72\),gr9
 4a0:	80 88 00 00 	nop
 4a4:	80 88 00 00 	nop
 4a8:	12 c8 f0 60 	ldi\.p @\(gr15,96\),gr9
 4ac:	80 88 00 00 	nop
 4b0:	80 88 00 00 	nop
 4b4:	80 88 00 00 	nop
 4b8:	92 fc f8 14 	setlos 0xf*fffff814,gr9
 4bc:	80 88 00 00 	nop
 4c0:	92 fc 08 14 	setlos 0x814,gr9
 4c4:	92 f8 00 00 	sethi hi\(0x0\),gr9
 4c8:	92 f4 f8 14 	setlo 0xf814,gr9
 4cc:	92 c8 f0 64 	ldi @\(gr15,100\),gr9
 4d0:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
 4d4:	92 c8 f0 1c 	ldi @\(gr15,28\),gr9
 4d8:	00 88 00 00 	nop\.p
 4dc:	80 88 00 00 	nop
 4e0:	92 c8 f0 98 	ldi @\(gr15,152\),gr9
 4e4:	80 88 00 00 	nop
 4e8:	00 88 00 00 	nop\.p
 4ec:	80 88 00 00 	nop
 4f0:	92 c8 f0 6c 	ldi @\(gr15,108\),gr9
 4f4:	80 88 00 00 	nop
 4f8:	00 88 00 00 	nop\.p
 4fc:	80 88 00 00 	nop
 500:	92 c8 f0 70 	ldi @\(gr15,112\),gr9
 504:	80 88 00 00 	nop
 508:	12 c8 f0 68 	ldi\.p @\(gr15,104\),gr9
 50c:	80 88 00 00 	nop
 510:	80 88 00 00 	nop
 514:	12 c8 f0 10 	ldi\.p @\(gr15,16\),gr9
 518:	80 88 00 00 	nop
 51c:	80 88 00 00 	nop
 520:	12 c8 f0 20 	ldi\.p @\(gr15,32\),gr9
 524:	80 88 00 00 	nop
 528:	80 88 00 00 	nop
 52c:	80 88 00 00 	nop
 530:	92 fc f8 24 	setlos 0xf*fffff824,gr9
 534:	80 88 00 00 	nop
 538:	92 fc 08 24 	setlos 0x824,gr9
 53c:	92 f8 00 00 	sethi hi\(0x0\),gr9
 540:	92 f4 f8 24 	setlo 0xf824,gr9
 544:	92 c8 f0 28 	ldi @\(gr15,40\),gr9
 548:	92 c8 f0 4c 	ldi @\(gr15,76\),gr9
 54c:	92 c8 f0 50 	ldi @\(gr15,80\),gr9
 550:	00 88 00 00 	nop\.p
 554:	80 88 00 00 	nop
 558:	92 c8 f0 74 	ldi @\(gr15,116\),gr9
 55c:	80 88 00 00 	nop
 560:	00 88 00 00 	nop\.p
 564:	80 88 00 00 	nop
 568:	92 c8 f0 88 	ldi @\(gr15,136\),gr9
 56c:	80 88 00 00 	nop
 570:	00 88 00 00 	nop\.p
 574:	80 88 00 00 	nop
 578:	92 c8 f0 8c 	ldi @\(gr15,140\),gr9
 57c:	80 88 00 00 	nop
 580:	12 c8 f0 2c 	ldi\.p @\(gr15,44\),gr9
 584:	80 88 00 00 	nop
 588:	80 88 00 00 	nop
 58c:	12 c8 f0 3c 	ldi\.p @\(gr15,60\),gr9
 590:	80 88 00 00 	nop
 594:	80 88 00 00 	nop
 598:	12 c8 f0 54 	ldi\.p @\(gr15,84\),gr9
 59c:	80 88 00 00 	nop
 5a0:	80 88 00 00 	nop
 5a4:	80 88 00 00 	nop
 5a8:	92 fc 00 04 	setlos 0x4,gr9
 5ac:	80 88 00 00 	nop
 5b0:	92 fc 10 04 	setlos 0x1004,gr9
 5b4:	92 f8 00 01 	sethi 0x1,gr9
 5b8:	92 f4 00 04 	setlo 0x4,gr9
 5bc:	92 c8 f0 30 	ldi @\(gr15,48\),gr9
 5c0:	92 c8 f0 40 	ldi @\(gr15,64\),gr9
 5c4:	92 c8 f0 58 	ldi @\(gr15,88\),gr9
 5c8:	00 88 00 00 	nop\.p
 5cc:	80 88 00 00 	nop
 5d0:	92 c8 f0 78 	ldi @\(gr15,120\),gr9
 5d4:	80 88 00 00 	nop
 5d8:	00 88 00 00 	nop\.p
 5dc:	80 88 00 00 	nop
 5e0:	92 c8 f0 80 	ldi @\(gr15,128\),gr9
 5e4:	80 88 00 00 	nop
 5e8:	00 88 00 00 	nop\.p
 5ec:	80 88 00 00 	nop
 5f0:	92 c8 f0 90 	ldi @\(gr15,144\),gr9
 5f4:	80 88 00 00 	nop
 5f8:	12 c8 f0 14 	ldi\.p @\(gr15,20\),gr9
 5fc:	80 88 00 00 	nop
 600:	80 88 00 00 	nop
 604:	12 c8 f0 18 	ldi\.p @\(gr15,24\),gr9
 608:	80 88 00 00 	nop
 60c:	80 88 00 00 	nop
 610:	12 c8 f0 24 	ldi\.p @\(gr15,36\),gr9
 614:	80 88 00 00 	nop
 618:	80 88 00 00 	nop
 61c:	92 c8 f0 34 	ldi @\(gr15,52\),gr9
 620:	92 c8 f0 5c 	ldi @\(gr15,92\),gr9
 624:	92 c8 f0 64 	ldi @\(gr15,100\),gr9
 628:	92 c8 f0 1c 	ldi @\(gr15,28\),gr9
 62c:	92 c8 f0 28 	ldi @\(gr15,40\),gr9
 630:	92 c8 f0 50 	ldi @\(gr15,80\),gr9
 634:	92 c8 f0 30 	ldi @\(gr15,48\),gr9
 638:	92 c8 f0 58 	ldi @\(gr15,88\),gr9
 63c:	80 88 00 00 	nop
 640:	92 c8 f0 44 	ldi @\(gr15,68\),gr9
 644:	80 88 00 00 	nop
 648:	80 88 00 00 	nop
 64c:	92 c8 f0 0c 	ldi @\(gr15,12\),gr9
Disassembly of section \.got:

000046e8 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    46f4:	00 00 10 11 	add\.p sp,gr17,gr0
			46f4: R_FRV_TLSOFF	\.tbss
    46f8:	00 00 10 13 	add\.p sp,gr19,gr0
			46f8: R_FRV_TLSOFF	\.tbss
    46fc:	00 00 00 03 	add\.p gr0,gr3,gr0
			46fc: R_FRV_TLSOFF	x
    4700:	00 00 10 03 	add\.p sp,gr3,gr0
			4700: R_FRV_TLSOFF	x
    4704:	00 01 00 11 	add\.p gr16,gr17,gr0
			4704: R_FRV_TLSOFF	\.tbss
    4708:	00 01 00 13 	add\.p gr16,gr19,gr0
			4708: R_FRV_TLSOFF	\.tbss
    470c:	00 01 00 03 	add\.p gr16,gr3,gr0
			470c: R_FRV_TLSOFF	x
    4710:	00 00 07 f1 	\*unknown\*
			4710: R_FRV_TLSOFF	\.tbss
    4714:	00 00 07 f3 	\*unknown\*
			4714: R_FRV_TLSOFF	\.tbss
    4718:	00 00 00 01 	add\.p gr0,sp,gr0
			4718: R_FRV_TLSOFF	x
    471c:	00 00 00 01 	add\.p gr0,sp,gr0
			471c: R_FRV_TLSOFF	\.tbss
    4720:	00 00 00 03 	add\.p gr0,gr3,gr0
			4720: R_FRV_TLSOFF	\.tbss
    4724:	00 00 17 f3 	\*unknown\*
			4724: R_FRV_TLSOFF	\.tbss
    4728:	00 00 10 01 	add\.p sp,sp,gr0
			4728: R_FRV_TLSOFF	x
    472c:	00 00 10 01 	add\.p sp,sp,gr0
			472c: R_FRV_TLSOFF	\.tbss
    4730:	00 00 10 03 	add\.p sp,gr3,gr0
			4730: R_FRV_TLSOFF	\.tbss
    4734:	00 00 17 f1 	\*unknown\*
			4734: R_FRV_TLSOFF	\.tbss
    4738:	00 01 07 f1 	\*unknown\*
			4738: R_FRV_TLSOFF	\.tbss
    473c:	00 01 07 f3 	\*unknown\*
			473c: R_FRV_TLSOFF	\.tbss
    4740:	00 01 00 01 	add\.p gr16,sp,gr0
			4740: R_FRV_TLSOFF	x
    4744:	00 01 00 01 	add\.p gr16,sp,gr0
			4744: R_FRV_TLSOFF	\.tbss
    4748:	00 01 00 03 	add\.p gr16,gr3,gr0
			4748: R_FRV_TLSOFF	\.tbss
    474c:	00 00 00 11 	add\.p gr0,gr17,gr0
			474c: R_FRV_TLSOFF	\.tbss
    4750:	00 00 00 13 	add\.p gr0,gr19,gr0
			4750: R_FRV_TLSOFF	\.tbss
    4754:	00 00 10 12 	add\.p sp,gr18,gr0
			4754: R_FRV_TLSOFF	\.tbss
    4758:	00 01 00 12 	add\.p gr16,gr18,gr0
			4758: R_FRV_TLSOFF	\.tbss
    475c:	00 00 07 f2 	\*unknown\*
			475c: R_FRV_TLSOFF	\.tbss
    4760:	00 00 00 02 	add\.p gr0,fp,gr0
			4760: R_FRV_TLSOFF	x
    4764:	00 00 00 02 	add\.p gr0,fp,gr0
			4764: R_FRV_TLSOFF	\.tbss
    4768:	00 00 10 02 	add\.p sp,fp,gr0
			4768: R_FRV_TLSOFF	x
    476c:	00 00 10 02 	add\.p sp,fp,gr0
			476c: R_FRV_TLSOFF	\.tbss
    4770:	00 00 17 f2 	\*unknown\*
			4770: R_FRV_TLSOFF	\.tbss
    4774:	00 01 07 f2 	\*unknown\*
			4774: R_FRV_TLSOFF	\.tbss
    4778:	00 01 00 02 	add\.p gr16,fp,gr0
			4778: R_FRV_TLSOFF	x
    477c:	00 01 00 02 	add\.p gr16,fp,gr0
			477c: R_FRV_TLSOFF	\.tbss
    4780:	00 00 00 12 	add\.p gr0,gr18,gr0
			4780: R_FRV_TLSOFF	\.tbss
