#source: notoc.s
#as: -a64
#ld: --no-plt-localentry --no-power10-stubs -T ext.lnk
#objdump: -d
#target: powerpc64*-*-*

.*

Disassembly of section \.text:

.* <.*\.long_branch\.f1>:
.*:	(18 00 41 f8|f8 41 00 18) 	std     r2,24\(r1\)
.*:	(7c 00 00 48|48 00 00 7c) 	b       .* <f1>

.* <.*\.long_branch\.g1>:
.*:	(18 00 41 f8|f8 41 00 18) 	std     r2,24\(r1\)
.*:	(8c 00 00 48|48 00 00 8c) 	b       .* <g1>

.* <.*\.plt_branch\.ext>:
.*:	(a6 02 88 7d|7d 88 02 a6) 	mflr    r12
.*:	(05 00 9f 42|42 9f 00 05) 	bcl     .*
.*:	(a6 02 68 7d|7d 68 02 a6) 	mflr    r11
.*:	(a6 03 88 7d|7d 88 03 a6) 	mtlr    r12
.*:	(ff 7f 80 3d|3d 80 7f ff) 	lis     r12,32767
.*:	(ff ff 8c 61|61 8c ff ff) 	ori     r12,r12,65535
.*:	(c6 07 9c 79|79 9c 07 c6) 	rldicr  r28,r12,32,31
.*:	(ff ef 8c 65|65 8c ef ff) 	oris    r12,r12,61439
.*:	(28 ff 8c 61|61 8c ff 28) 	ori     r12,r12,65320
.*:	(14 62 8b 7d|7d 8b 62 14) 	add     r12,r11,r12
.*:	(a6 03 89 7d|7d 89 03 a6) 	mtctr   r12
.*:	(20 04 80 4e|4e 80 04 20) 	bctr

.* <.*\.long_branch\.f2>:
.*:	(a6 02 88 7d|7d 88 02 a6) 	mflr    r12
.*:	(05 00 9f 42|42 9f 00 05) 	bcl     .*
.*:	(a6 02 68 7d|7d 68 02 a6) 	mflr    r11
.*:	(a6 03 88 7d|7d 88 03 a6) 	mtlr    r12
.*:	(64 00 8b 39|39 8b 00 64) 	addi    r12,r11,100
.*:	(58 00 00 48|48 00 00 58) 	b       .* <f2>

.* <.*\.long_branch\.g2>:
.*:	(a6 02 88 7d|7d 88 02 a6) 	mflr    r12
.*:	(05 00 9f 42|42 9f 00 05) 	bcl     .*
.*:	(a6 02 68 7d|7d 68 02 a6) 	mflr    r11
.*:	(a6 03 88 7d|7d 88 03 a6) 	mtlr    r12
.*:	(80 00 8b 39|39 8b 00 80) 	addi    r12,r11,128
.*:	(74 00 00 48|48 00 00 74) 	b       .* <g2>
	\.\.\.

.* <f1>:
.*:	(01 00 00 48|48 00 00 01) 	bl      .* <f1>
.*:	(bd ff ff 4b|4b ff ff bd) 	bl      .* <.*\.long_branch\.f2>
.*:	(11 00 00 48|48 00 00 11) 	bl      .* <g1>
.*:	(cd ff ff 4b|4b ff ff cd) 	bl      .* <.*\.long_branch\.g2>
.*:	(81 ff ff 4b|4b ff ff 81) 	bl      .* <.*\.plt_branch\.ext>
.*:	(20 00 80 4e|4e 80 00 20) 	blr

.* <g1>:
.*:	(a9 ff ff 4b|4b ff ff a9) 	bl      .* <.*\.long_branch\.f2>
.*:	(e5 ff ff 4b|4b ff ff e5) 	bl      .* <f1>
.*:	(b9 ff ff 4b|4b ff ff b9) 	bl      .* <.*\.long_branch\.g2>
.*:	(f5 ff ff 4b|4b ff ff f5) 	bl      .* <g1>
.*:	(20 00 80 4e|4e 80 00 20) 	blr

.* <f2>:
.*:	(01 10 40 3c|3c 40 10 01) 	lis     r2,4097
.*:	(00 80 42 38|38 42 80 00) 	addi    r2,r2,-32768
.*:	(4d ff ff 4b|4b ff ff 4d) 	bl      .* <.*\.long_branch\.f1>
.*:	(18 00 41 e8|e8 41 00 18) 	ld      r2,24\(r1\)
.*:	(f9 ff ff 4b|4b ff ff f9) 	bl      .* <f2\+0x8>
.*:	(00 00 00 60|60 00 00 00) 	nop
.*:	(45 ff ff 4b|4b ff ff 45) 	bl      .* <.*\.long_branch\.g1>
.*:	(18 00 41 e8|e8 41 00 18) 	ld      r2,24\(r1\)
.*:	(1d 00 00 48|48 00 00 1d) 	bl      .* <g2\+0x8>
.*:	(00 00 00 60|60 00 00 00) 	nop
.*:	(3d ff ff 4b|4b ff ff 3d) 	bl      .* <.*\.plt_branch\.ext>
.*:	(00 00 00 60|60 00 00 00) 	nop
.*:	(20 00 80 4e|4e 80 00 20) 	blr

.* <g2>:
.*:	(01 10 40 3c|3c 40 10 01) 	lis     r2,4097
.*:	(00 80 42 38|38 42 80 00) 	addi    r2,r2,-32768
.*:	(cd ff ff 4b|4b ff ff cd) 	bl      .* <f2\+0x8>
.*:	(00 00 00 60|60 00 00 00) 	nop
.*:	(11 ff ff 4b|4b ff ff 11) 	bl      .* <.*\.long_branch\.f1>
.*:	(18 00 41 e8|e8 41 00 18) 	ld      r2,24\(r1\)
.*:	(f1 ff ff 4b|4b ff ff f1) 	bl      .* <g2\+0x8>
.*:	(00 00 00 60|60 00 00 00) 	nop
.*:	(09 ff ff 4b|4b ff ff 09) 	bl      .* <.*\.long_branch\.g1>
.*:	(18 00 41 e8|e8 41 00 18) 	ld      r2,24\(r1\)
.*:	(20 00 80 4e|4e 80 00 20) 	blr

.* <_start>:
.*:	(00 00 00 48|48 00 00 00) 	b       .* <_start>

Disassembly of section \.text\.ext:

8000000000000000 <ext>:
8000000000000000:	(01 10 40 3c|3c 40 10 01) 	lis     r2,4097
8000000000000004:	(00 80 42 38|38 42 80 00) 	addi    r2,r2,-32768
8000000000000008:	(00 00 00 60|60 00 00 00) 	nop
800000000000000c:	(20 00 80 4e|4e 80 00 20) 	blr
