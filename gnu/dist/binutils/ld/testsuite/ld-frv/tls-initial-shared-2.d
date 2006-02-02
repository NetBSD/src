#name: FRV TLS relocs with addends, shared linking with static TLS
#source: tls-2.s
#as: --defsym static_tls=1
#objdump: -DR -j .text -j .got -j .plt
#ld: -shared tmpdir/tls-1-dep.so --version-script tls-1-shared.lds

.*:     file format elf.*frv.*

Disassembly of section \.plt:

00000464 <\.plt>:
 464:	92 c8 f0 5c 	ldi @\(gr15,92\),gr9
 468:	c0 3a 40 00 	bralr
 46c:	90 cc ff e8 	lddi @\(gr15,-24\),gr8
 470:	80 30 80 00 	jmpl @\(gr8,gr0\)
 474:	92 c8 f0 78 	ldi @\(gr15,120\),gr9
 478:	c0 3a 40 00 	bralr
 47c:	90 cc ff f8 	lddi @\(gr15,-8\),gr8
 480:	80 30 80 00 	jmpl @\(gr8,gr0\)
Disassembly of section \.text:

00000484 <_start>:
 484:	92 c8 f0 30 	ldi @\(gr15,48\),gr9
 488:	92 c8 f0 78 	ldi @\(gr15,120\),gr9
 48c:	92 c8 f0 4c 	ldi @\(gr15,76\),gr9
 490:	00 88 00 00 	nop\.p
 494:	80 88 00 00 	nop
 498:	92 c8 f0 70 	ldi @\(gr15,112\),gr9
 49c:	80 88 00 00 	nop
 4a0:	00 88 00 00 	nop\.p
 4a4:	80 88 00 00 	nop
 4a8:	92 c8 f0 7c 	ldi @\(gr15,124\),gr9
 4ac:	80 88 00 00 	nop
 4b0:	00 88 00 00 	nop\.p
 4b4:	80 88 00 00 	nop
 4b8:	92 c8 f0 8c 	ldi @\(gr15,140\),gr9
 4bc:	80 88 00 00 	nop
 4c0:	12 c8 f0 34 	ldi\.p @\(gr15,52\),gr9
 4c4:	80 88 00 00 	nop
 4c8:	80 88 00 00 	nop
 4cc:	12 c8 f0 3c 	ldi\.p @\(gr15,60\),gr9
 4d0:	80 88 00 00 	nop
 4d4:	80 88 00 00 	nop
 4d8:	12 c8 f0 50 	ldi\.p @\(gr15,80\),gr9
 4dc:	80 88 00 00 	nop
 4e0:	80 88 00 00 	nop
 4e4:	80 88 00 00 	nop
 4e8:	92 fc f8 14 	setlos 0xf*fffff814,gr9
 4ec:	80 88 00 00 	nop
 4f0:	92 fc 08 14 	setlos 0x814,gr9
 4f4:	92 f8 00 00 	sethi hi\(0x0\),gr9
 4f8:	92 f4 f8 14 	setlo 0xf814,gr9
 4fc:	92 c8 f0 54 	ldi @\(gr15,84\),gr9
 500:	92 c8 f0 5c 	ldi @\(gr15,92\),gr9
 504:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
 508:	00 88 00 00 	nop\.p
 50c:	80 88 00 00 	nop
 510:	92 c8 f0 90 	ldi @\(gr15,144\),gr9
 514:	80 88 00 00 	nop
 518:	00 88 00 00 	nop\.p
 51c:	80 88 00 00 	nop
 520:	92 c8 f0 60 	ldi @\(gr15,96\),gr9
 524:	80 88 00 00 	nop
 528:	00 88 00 00 	nop\.p
 52c:	80 88 00 00 	nop
 530:	92 c8 f0 64 	ldi @\(gr15,100\),gr9
 534:	80 88 00 00 	nop
 538:	12 c8 f0 58 	ldi\.p @\(gr15,88\),gr9
 53c:	80 88 00 00 	nop
 540:	80 88 00 00 	nop
 544:	12 c8 f0 0c 	ldi\.p @\(gr15,12\),gr9
 548:	80 88 00 00 	nop
 54c:	80 88 00 00 	nop
 550:	12 c8 f0 1c 	ldi\.p @\(gr15,28\),gr9
 554:	80 88 00 00 	nop
 558:	80 88 00 00 	nop
 55c:	80 88 00 00 	nop
 560:	92 fc f8 24 	setlos 0xf*fffff824,gr9
 564:	80 88 00 00 	nop
 568:	92 fc 08 24 	setlos 0x824,gr9
 56c:	92 f8 00 00 	sethi hi\(0x0\),gr9
 570:	92 f4 f8 24 	setlo 0xf824,gr9
 574:	92 c8 f0 24 	ldi @\(gr15,36\),gr9
 578:	fe 3f ff c1 	call 47c <i\+0x46c>
 57c:	92 c8 f0 40 	ldi @\(gr15,64\),gr9
 580:	00 88 00 00 	nop\.p
 584:	80 88 00 00 	nop
 588:	92 c8 f0 68 	ldi @\(gr15,104\),gr9
 58c:	80 88 00 00 	nop
 590:	00 88 00 00 	nop\.p
 594:	80 88 00 00 	nop
 598:	92 c8 f0 80 	ldi @\(gr15,128\),gr9
 59c:	80 88 00 00 	nop
 5a0:	00 88 00 00 	nop\.p
 5a4:	80 88 00 00 	nop
 5a8:	92 c8 f0 84 	ldi @\(gr15,132\),gr9
 5ac:	80 88 00 00 	nop
 5b0:	12 c8 f0 28 	ldi\.p @\(gr15,40\),gr9
 5b4:	80 88 00 00 	nop
 5b8:	80 88 00 00 	nop
 5bc:	12 c8 f0 38 	ldi\.p @\(gr15,56\),gr9
 5c0:	80 88 00 00 	nop
 5c4:	80 88 00 00 	nop
 5c8:	12 c8 f0 44 	ldi\.p @\(gr15,68\),gr9
 5cc:	80 88 00 00 	nop
 5d0:	80 88 00 00 	nop
 5d4:	80 88 00 00 	nop
 5d8:	92 fc 00 04 	setlos 0x4,gr9
 5dc:	80 88 00 00 	nop
 5e0:	92 fc 10 04 	setlos 0x1004,gr9
 5e4:	92 f8 00 01 	sethi 0x1,gr9
 5e8:	92 f4 00 04 	setlo 0x4,gr9
 5ec:	92 c8 f0 2c 	ldi @\(gr15,44\),gr9
 5f0:	fe 3f ff 9f 	call 46c <i\+0x45c>
 5f4:	92 c8 f0 48 	ldi @\(gr15,72\),gr9
 5f8:	00 88 00 00 	nop\.p
 5fc:	80 88 00 00 	nop
 600:	92 c8 f0 6c 	ldi @\(gr15,108\),gr9
 604:	80 88 00 00 	nop
 608:	00 88 00 00 	nop\.p
 60c:	80 88 00 00 	nop
 610:	92 c8 f0 74 	ldi @\(gr15,116\),gr9
 614:	80 88 00 00 	nop
 618:	00 88 00 00 	nop\.p
 61c:	80 88 00 00 	nop
 620:	92 c8 f0 88 	ldi @\(gr15,136\),gr9
 624:	80 88 00 00 	nop
 628:	12 c8 f0 10 	ldi\.p @\(gr15,16\),gr9
 62c:	80 88 00 00 	nop
 630:	80 88 00 00 	nop
 634:	12 c8 f0 14 	ldi\.p @\(gr15,20\),gr9
 638:	80 88 00 00 	nop
 63c:	80 88 00 00 	nop
 640:	12 c8 f0 20 	ldi\.p @\(gr15,32\),gr9
 644:	80 88 00 00 	nop
 648:	80 88 00 00 	nop
 64c:	92 c8 f0 30 	ldi @\(gr15,48\),gr9
 650:	92 c8 f0 4c 	ldi @\(gr15,76\),gr9
 654:	92 c8 f0 54 	ldi @\(gr15,84\),gr9
 658:	92 c8 f0 18 	ldi @\(gr15,24\),gr9
 65c:	92 c8 f0 24 	ldi @\(gr15,36\),gr9
 660:	92 c8 f0 40 	ldi @\(gr15,64\),gr9
 664:	92 c8 f0 2c 	ldi @\(gr15,44\),gr9
 668:	92 c8 f0 48 	ldi @\(gr15,72\),gr9
 66c:	80 88 00 00 	nop
 670:	92 c8 f0 78 	ldi @\(gr15,120\),gr9
 674:	80 88 00 00 	nop
 678:	80 88 00 00 	nop
 67c:	92 c8 f0 5c 	ldi @\(gr15,92\),gr9
Disassembly of section \.got:

00004718 <_GLOBAL_OFFSET_TABLE_-0x20>:
    4718:	00 00 00 00 	add\.p gr0,gr0,gr0
			4718: R_FRV_TLSDESC_VALUE	\.tbss
    471c:	00 00 10 11 	add\.p sp,gr17,gr0
    4720:	00 00 00 00 	add\.p gr0,gr0,gr0
			4720: R_FRV_TLSDESC_VALUE	x
    4724:	00 00 10 01 	add\.p sp,sp,gr0
    4728:	00 00 00 00 	add\.p gr0,gr0,gr0
			4728: R_FRV_TLSDESC_VALUE	\.tbss
    472c:	00 00 10 01 	add\.p sp,sp,gr0
    4730:	00 00 00 00 	add\.p gr0,gr0,gr0
			4730: R_FRV_TLSDESC_VALUE	\.tbss
    4734:	00 00 17 f1 	\*unknown\*

00004738 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
    4744:	00 00 10 13 	add\.p sp,gr19,gr0
			4744: R_FRV_TLSOFF	\.tbss
    4748:	00 00 00 03 	add\.p gr0,gr3,gr0
			4748: R_FRV_TLSOFF	x
    474c:	00 00 10 03 	add\.p sp,gr3,gr0
			474c: R_FRV_TLSOFF	x
    4750:	00 01 00 11 	add\.p gr16,gr17,gr0
			4750: R_FRV_TLSOFF	\.tbss
    4754:	00 01 00 13 	add\.p gr16,gr19,gr0
			4754: R_FRV_TLSOFF	\.tbss
    4758:	00 01 00 03 	add\.p gr16,gr3,gr0
			4758: R_FRV_TLSOFF	x
    475c:	00 00 07 f1 	\*unknown\*
			475c: R_FRV_TLSOFF	\.tbss
    4760:	00 00 07 f3 	\*unknown\*
			4760: R_FRV_TLSOFF	\.tbss
    4764:	00 00 00 01 	add\.p gr0,sp,gr0
			4764: R_FRV_TLSOFF	x
    4768:	00 00 00 01 	add\.p gr0,sp,gr0
			4768: R_FRV_TLSOFF	\.tbss
    476c:	00 00 00 03 	add\.p gr0,gr3,gr0
			476c: R_FRV_TLSOFF	\.tbss
    4770:	00 00 17 f3 	\*unknown\*
			4770: R_FRV_TLSOFF	\.tbss
    4774:	00 00 10 03 	add\.p sp,gr3,gr0
			4774: R_FRV_TLSOFF	\.tbss
    4778:	00 01 07 f1 	\*unknown\*
			4778: R_FRV_TLSOFF	\.tbss
    477c:	00 01 07 f3 	\*unknown\*
			477c: R_FRV_TLSOFF	\.tbss
    4780:	00 01 00 01 	add\.p gr16,sp,gr0
			4780: R_FRV_TLSOFF	x
    4784:	00 01 00 01 	add\.p gr16,sp,gr0
			4784: R_FRV_TLSOFF	\.tbss
    4788:	00 01 00 03 	add\.p gr16,gr3,gr0
			4788: R_FRV_TLSOFF	\.tbss
    478c:	00 00 00 11 	add\.p gr0,gr17,gr0
			478c: R_FRV_TLSOFF	\.tbss
    4790:	00 00 00 13 	add\.p gr0,gr19,gr0
			4790: R_FRV_TLSOFF	\.tbss
    4794:	00 00 10 11 	add\.p sp,gr17,gr0
			4794: R_FRV_TLSOFF	\.tbss
    4798:	00 00 10 12 	add\.p sp,gr18,gr0
			4798: R_FRV_TLSOFF	\.tbss
    479c:	00 01 00 12 	add\.p gr16,gr18,gr0
			479c: R_FRV_TLSOFF	\.tbss
    47a0:	00 00 07 f2 	\*unknown\*
			47a0: R_FRV_TLSOFF	\.tbss
    47a4:	00 00 00 02 	add\.p gr0,fp,gr0
			47a4: R_FRV_TLSOFF	x
    47a8:	00 00 00 02 	add\.p gr0,fp,gr0
			47a8: R_FRV_TLSOFF	\.tbss
    47ac:	00 00 10 02 	add\.p sp,fp,gr0
			47ac: R_FRV_TLSOFF	x
    47b0:	00 00 10 01 	add\.p sp,sp,gr0
			47b0: R_FRV_TLSOFF	\.tbss
    47b4:	00 00 10 02 	add\.p sp,fp,gr0
			47b4: R_FRV_TLSOFF	\.tbss
    47b8:	00 00 17 f2 	\*unknown\*
			47b8: R_FRV_TLSOFF	\.tbss
    47bc:	00 01 07 f2 	\*unknown\*
			47bc: R_FRV_TLSOFF	\.tbss
    47c0:	00 01 00 02 	add\.p gr16,fp,gr0
			47c0: R_FRV_TLSOFF	x
    47c4:	00 01 00 02 	add\.p gr16,fp,gr0
			47c4: R_FRV_TLSOFF	\.tbss
    47c8:	00 00 00 12 	add\.p gr0,gr18,gr0
			47c8: R_FRV_TLSOFF	\.tbss
