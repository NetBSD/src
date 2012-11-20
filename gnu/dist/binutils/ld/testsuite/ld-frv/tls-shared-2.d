#name: FRV TLS relocs with addends, shared linking
#source: tls-2.s
#objdump: -DR -j .text -j .got -j .plt
#ld: -shared tmpdir/tls-1-dep.so --version-script tls-1-shared.lds

.*:     file format elf.*frv.*

Disassembly of section \.plt:

00000454 <\.plt>:
 454:	90 cc f0 10 	lddi @\(gr15,16\),gr8
 458:	80 30 80 00 	jmpl @\(gr8,gr0\)
 45c:	90 cc f0 30 	lddi @\(gr15,48\),gr8
 460:	80 30 80 00 	jmpl @\(gr8,gr0\)
 464:	90 cc f0 40 	lddi @\(gr15,64\),gr8
 468:	80 30 80 00 	jmpl @\(gr8,gr0\)
 46c:	90 cc f0 50 	lddi @\(gr15,80\),gr8
 470:	80 30 80 00 	jmpl @\(gr8,gr0\)
 474:	90 cc f0 58 	lddi @\(gr15,88\),gr8
 478:	80 30 80 00 	jmpl @\(gr8,gr0\)
 47c:	90 cc f0 68 	lddi @\(gr15,104\),gr8
 480:	80 30 80 00 	jmpl @\(gr8,gr0\)
 484:	90 cc ff a8 	lddi @\(gr15,-88\),gr8
 488:	80 30 80 00 	jmpl @\(gr8,gr0\)
 48c:	90 cc ff b0 	lddi @\(gr15,-80\),gr8
 490:	80 30 80 00 	jmpl @\(gr8,gr0\)
 494:	90 cc ff c8 	lddi @\(gr15,-56\),gr8
 498:	80 30 80 00 	jmpl @\(gr8,gr0\)
 49c:	90 cc ff d8 	lddi @\(gr15,-40\),gr8
 4a0:	80 30 80 00 	jmpl @\(gr8,gr0\)
 4a4:	90 cc ff e0 	lddi @\(gr15,-32\),gr8
 4a8:	80 30 80 00 	jmpl @\(gr8,gr0\)
 4ac:	90 cc ff f0 	lddi @\(gr15,-16\),gr8
 4b0:	80 30 80 00 	jmpl @\(gr8,gr0\)
Disassembly of section \.text:

000004b4 <_start>:
 4b4:	fe 3f ff f0 	call 474 <i\+0x464>
 4b8:	fe 3f ff f5 	call 48c <i\+0x47c>
 4bc:	fe 3f ff fa 	call 4a4 <i\+0x494>
 4c0:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 4c4:	9c f4 00 98 	setlo 0x98,gr14
 4c8:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 4cc:	82 30 80 00 	calll @\(gr8,gr0\)
 4d0:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 4d4:	9c f4 00 a8 	setlo 0xa8,gr14
 4d8:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 4dc:	82 30 80 00 	calll @\(gr8,gr0\)
 4e0:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 4e4:	9c f4 00 c0 	setlo 0xc0,gr14
 4e8:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 4ec:	82 30 80 00 	calll @\(gr8,gr0\)
 4f0:	10 cc f0 60 	lddi\.p @\(gr15,96\),gr8
 4f4:	9c fc 00 60 	setlos 0x60,gr14
 4f8:	82 30 80 00 	calll @\(gr8,gr0\)
 4fc:	10 cc ff b8 	lddi\.p @\(gr15,-72\),gr8
 500:	9c fc ff b8 	setlos 0xf*ffffffb8,gr14
 504:	82 30 80 00 	calll @\(gr8,gr0\)
 508:	10 cc ff e8 	lddi\.p @\(gr15,-24\),gr8
 50c:	9c fc ff e8 	setlos 0xf*ffffffe8,gr14
 510:	82 30 80 00 	calll @\(gr8,gr0\)
 514:	80 88 00 00 	nop
 518:	92 fc f8 14 	setlos 0xf*fffff814,gr9
 51c:	80 88 00 00 	nop
 520:	92 fc 08 14 	setlos 0x814,gr9
 524:	92 f8 00 00 	sethi hi\(0x0\),gr9
 528:	92 f4 f8 14 	setlo 0xf814,gr9
 52c:	fe 3f ff e0 	call 4ac <i\+0x49c>
 530:	fe 3f ff c9 	call 454 <i\+0x444>
 534:	fe 3f ff ca 	call 45c <i\+0x44c>
 538:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 53c:	9c f4 00 c8 	setlo 0xc8,gr14
 540:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 544:	82 30 80 00 	calll @\(gr8,gr0\)
 548:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 54c:	9c f4 00 78 	setlo 0x78,gr14
 550:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 554:	82 30 80 00 	calll @\(gr8,gr0\)
 558:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 55c:	9c f4 00 88 	setlo 0x88,gr14
 560:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 564:	82 30 80 00 	calll @\(gr8,gr0\)
 568:	10 cc ff f8 	lddi\.p @\(gr15,-8\),gr8
 56c:	9c fc ff f8 	setlos 0xf*fffffff8,gr14
 570:	82 30 80 00 	calll @\(gr8,gr0\)
 574:	10 cc f0 18 	lddi\.p @\(gr15,24\),gr8
 578:	9c fc 00 18 	setlos 0x18,gr14
 57c:	82 30 80 00 	calll @\(gr8,gr0\)
 580:	10 cc f0 38 	lddi\.p @\(gr15,56\),gr8
 584:	9c fc 00 38 	setlos 0x38,gr14
 588:	82 30 80 00 	calll @\(gr8,gr0\)
 58c:	80 88 00 00 	nop
 590:	92 fc f8 24 	setlos 0xf*fffff824,gr9
 594:	80 88 00 00 	nop
 598:	92 fc 08 24 	setlos 0x824,gr9
 59c:	92 f8 00 00 	sethi hi\(0x0\),gr9
 5a0:	92 f4 f8 24 	setlo 0xf824,gr9
 5a4:	fe 3f ff b0 	call 464 <i\+0x454>
 5a8:	fe 3f ff b5 	call 47c <i\+0x46c>
 5ac:	fe 3f ff ba 	call 494 <i\+0x484>
 5b0:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 5b4:	9c f4 00 90 	setlo 0x90,gr14
 5b8:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 5bc:	82 30 80 00 	calll @\(gr8,gr0\)
 5c0:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 5c4:	9c f4 00 a0 	setlo 0xa0,gr14
 5c8:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 5cc:	82 30 80 00 	calll @\(gr8,gr0\)
 5d0:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 5d4:	9c f4 00 b8 	setlo 0xb8,gr14
 5d8:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 5dc:	82 30 80 00 	calll @\(gr8,gr0\)
 5e0:	10 cc f0 48 	lddi\.p @\(gr15,72\),gr8
 5e4:	9c fc 00 48 	setlos 0x48,gr14
 5e8:	82 30 80 00 	calll @\(gr8,gr0\)
 5ec:	10 cc ff a0 	lddi\.p @\(gr15,-96\),gr8
 5f0:	9c fc ff a0 	setlos 0xf*ffffffa0,gr14
 5f4:	82 30 80 00 	calll @\(gr8,gr0\)
 5f8:	10 cc ff d0 	lddi\.p @\(gr15,-48\),gr8
 5fc:	9c fc ff d0 	setlos 0xf*ffffffd0,gr14
 600:	82 30 80 00 	calll @\(gr8,gr0\)
 604:	80 88 00 00 	nop
 608:	92 fc 00 04 	setlos 0x4,gr9
 60c:	80 88 00 00 	nop
 610:	92 fc 10 04 	setlos 0x1004,gr9
 614:	92 f8 00 01 	sethi 0x1,gr9
 618:	92 f4 00 04 	setlo 0x4,gr9
 61c:	fe 3f ff 94 	call 46c <i\+0x45c>
 620:	fe 3f ff 99 	call 484 <i\+0x474>
 624:	fe 3f ff 9e 	call 49c <i\+0x48c>
 628:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 62c:	9c f4 00 b0 	setlo 0xb0,gr14
 630:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 634:	82 30 80 00 	calll @\(gr8,gr0\)
 638:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 63c:	9c f4 00 80 	setlo 0x80,gr14
 640:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 644:	82 30 80 00 	calll @\(gr8,gr0\)
 648:	1c f8 00 00 	sethi\.p hi\(0x0\),gr14
 64c:	9c f4 00 70 	setlo 0x70,gr14
 650:	90 08 f1 4e 	ldd @\(gr15,gr14\),gr8
 654:	82 30 80 00 	calll @\(gr8,gr0\)
 658:	10 cc f0 20 	lddi\.p @\(gr15,32\),gr8
 65c:	9c fc 00 20 	setlos 0x20,gr14
 660:	82 30 80 00 	calll @\(gr8,gr0\)
 664:	10 cc f0 28 	lddi\.p @\(gr15,40\),gr8
 668:	9c fc 00 28 	setlos 0x28,gr14
 66c:	82 30 80 00 	calll @\(gr8,gr0\)
 670:	10 cc ff c0 	lddi\.p @\(gr15,-64\),gr8
 674:	9c fc ff c0 	setlos 0xf*ffffffc0,gr14
 678:	82 30 80 00 	calll @\(gr8,gr0\)
Disassembly of section \.got:

00004700 <_GLOBAL_OFFSET_TABLE_-0x60>:
    4700:	00 00 00 00 	add\.p gr0,gr0,gr0
			4700: R_FRV_TLSDESC_VALUE	\.tbss
    4704:	00 00 17 f3 	\*unknown\*
    4708:	00 00 00 00 	add\.p gr0,gr0,gr0
			4708: R_FRV_TLSDESC_VALUE	x
    470c:	00 00 10 01 	add\.p sp,sp,gr0
    4710:	00 00 00 00 	add\.p gr0,gr0,gr0
			4710: R_FRV_TLSDESC_VALUE	\.tbss
    4714:	00 00 10 01 	add\.p sp,sp,gr0
    4718:	00 00 00 00 	add\.p gr0,gr0,gr0
			4718: R_FRV_TLSDESC_VALUE	\.tbss
    471c:	00 00 10 03 	add\.p sp,gr3,gr0
    4720:	00 00 00 00 	add\.p gr0,gr0,gr0
			4720: R_FRV_TLSDESC_VALUE	x
    4724:	00 01 00 03 	add\.p gr16,gr3,gr0
    4728:	00 00 00 00 	add\.p gr0,gr0,gr0
			4728: R_FRV_TLSDESC_VALUE	\.tbss
    472c:	00 01 07 f1 	\*unknown\*
    4730:	00 00 00 00 	add\.p gr0,gr0,gr0
			4730: R_FRV_TLSDESC_VALUE	\.tbss
    4734:	00 01 07 f3 	\*unknown\*
    4738:	00 00 00 00 	add\.p gr0,gr0,gr0
			4738: R_FRV_TLSDESC_VALUE	x
    473c:	00 01 00 01 	add\.p gr16,sp,gr0
    4740:	00 00 00 00 	add\.p gr0,gr0,gr0
			4740: R_FRV_TLSDESC_VALUE	\.tbss
    4744:	00 01 00 01 	add\.p gr16,sp,gr0
    4748:	00 00 00 00 	add\.p gr0,gr0,gr0
			4748: R_FRV_TLSDESC_VALUE	\.tbss
    474c:	00 01 00 03 	add\.p gr16,gr3,gr0
    4750:	00 00 00 00 	add\.p gr0,gr0,gr0
			4750: R_FRV_TLSDESC_VALUE	\.tbss
    4754:	00 00 00 11 	add\.p gr0,gr17,gr0
    4758:	00 00 00 00 	add\.p gr0,gr0,gr0
			4758: R_FRV_TLSDESC_VALUE	\.tbss
    475c:	00 00 00 13 	add\.p gr0,gr19,gr0

00004760 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
			4770: R_FRV_TLSDESC_VALUE	\.tbss
    4774:	00 00 10 11 	add\.p sp,gr17,gr0
    4778:	00 00 00 00 	add\.p gr0,gr0,gr0
			4778: R_FRV_TLSDESC_VALUE	\.tbss
    477c:	00 00 10 13 	add\.p sp,gr19,gr0
    4780:	00 00 00 00 	add\.p gr0,gr0,gr0
			4780: R_FRV_TLSDESC_VALUE	x
    4784:	00 00 00 03 	add\.p gr0,gr3,gr0
    4788:	00 00 00 00 	add\.p gr0,gr0,gr0
			4788: R_FRV_TLSDESC_VALUE	x
    478c:	00 00 10 03 	add\.p sp,gr3,gr0
    4790:	00 00 00 00 	add\.p gr0,gr0,gr0
			4790: R_FRV_TLSDESC_VALUE	\.tbss
    4794:	00 01 00 11 	add\.p gr16,gr17,gr0
    4798:	00 00 00 00 	add\.p gr0,gr0,gr0
			4798: R_FRV_TLSDESC_VALUE	\.tbss
    479c:	00 01 00 13 	add\.p gr16,gr19,gr0
    47a0:	00 00 00 00 	add\.p gr0,gr0,gr0
			47a0: R_FRV_TLSDESC_VALUE	\.tbss
    47a4:	00 00 07 f1 	\*unknown\*
    47a8:	00 00 00 00 	add\.p gr0,gr0,gr0
			47a8: R_FRV_TLSDESC_VALUE	\.tbss
    47ac:	00 00 07 f3 	\*unknown\*
    47b0:	00 00 00 00 	add\.p gr0,gr0,gr0
			47b0: R_FRV_TLSDESC_VALUE	x
    47b4:	00 00 00 01 	add\.p gr0,sp,gr0
    47b8:	00 00 00 00 	add\.p gr0,gr0,gr0
			47b8: R_FRV_TLSDESC_VALUE	\.tbss
    47bc:	00 00 00 01 	add\.p gr0,sp,gr0
    47c0:	00 00 00 00 	add\.p gr0,gr0,gr0
			47c0: R_FRV_TLSDESC_VALUE	\.tbss
    47c4:	00 00 00 03 	add\.p gr0,gr3,gr0
    47c8:	00 00 00 00 	add\.p gr0,gr0,gr0
			47c8: R_FRV_TLSDESC_VALUE	\.tbss
    47cc:	00 00 17 f1 	\*unknown\*
    47d0:	00 00 00 00 	add\.p gr0,gr0,gr0
			47d0: R_FRV_TLSDESC_VALUE	x
    47d4:	00 01 00 02 	add\.p gr16,fp,gr0
    47d8:	00 00 00 00 	add\.p gr0,gr0,gr0
			47d8: R_FRV_TLSDESC_VALUE	\.tbss
    47dc:	00 00 10 12 	add\.p sp,gr18,gr0
    47e0:	00 00 00 00 	add\.p gr0,gr0,gr0
			47e0: R_FRV_TLSDESC_VALUE	x
    47e4:	00 00 10 02 	add\.p sp,fp,gr0
    47e8:	00 00 00 00 	add\.p gr0,gr0,gr0
			47e8: R_FRV_TLSDESC_VALUE	\.tbss
    47ec:	00 01 00 12 	add\.p gr16,gr18,gr0
    47f0:	00 00 00 00 	add\.p gr0,gr0,gr0
			47f0: R_FRV_TLSDESC_VALUE	\.tbss
    47f4:	00 00 07 f2 	\*unknown\*
    47f8:	00 00 00 00 	add\.p gr0,gr0,gr0
			47f8: R_FRV_TLSDESC_VALUE	\.tbss
    47fc:	00 00 00 02 	add\.p gr0,fp,gr0
    4800:	00 00 00 00 	add\.p gr0,gr0,gr0
			4800: R_FRV_TLSDESC_VALUE	\.tbss
    4804:	00 00 17 f2 	\*unknown\*
    4808:	00 00 00 00 	add\.p gr0,gr0,gr0
			4808: R_FRV_TLSDESC_VALUE	\.tbss
    480c:	00 00 10 02 	add\.p sp,fp,gr0
    4810:	00 00 00 00 	add\.p gr0,gr0,gr0
			4810: R_FRV_TLSDESC_VALUE	x
    4814:	00 00 00 02 	add\.p gr0,fp,gr0
    4818:	00 00 00 00 	add\.p gr0,gr0,gr0
			4818: R_FRV_TLSDESC_VALUE	\.tbss
    481c:	00 01 07 f2 	\*unknown\*
    4820:	00 00 00 00 	add\.p gr0,gr0,gr0
			4820: R_FRV_TLSDESC_VALUE	\.tbss
    4824:	00 01 00 02 	add\.p gr16,fp,gr0
    4828:	00 00 00 00 	add\.p gr0,gr0,gr0
			4828: R_FRV_TLSDESC_VALUE	\.tbss
    482c:	00 00 00 12 	add\.p gr0,gr18,gr0
