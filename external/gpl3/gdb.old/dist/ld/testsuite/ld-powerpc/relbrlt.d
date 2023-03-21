#source: relbrlt.s
#as: -a64
#ld: -melf64ppc --no-plt-align --no-ld-generated-unwind-info --emit-relocs
#objdump: -Dr

.*

Disassembly of section \.text:

0*100000c0 <_start>:
[0-9a-f	 ]*:	(49 bf 00 2d|2d 00 bf 49) 	bl      .*
[0-9a-f	 ]*: R_PPC64_REL24	\.text\+0x37e003c
[0-9a-f	 ]*:	(60 00 00 00|00 00 00 60) 	nop
[0-9a-f	 ]*:	(49 bf 00 19|19 00 bf 49) 	bl      .*
[0-9a-f	 ]*: R_PPC64_REL24	\.text\+0x3bf0020
[0-9a-f	 ]*:	(60 00 00 00|00 00 00 60) 	nop
[0-9a-f	 ]*:	(49 bf 00 21|21 00 bf 49) 	bl      .*
[0-9a-f	 ]*: R_PPC64_REL24	\.text\+0x57e0024
[0-9a-f	 ]*:	(60 00 00 00|00 00 00 60) 	nop
[0-9a-f	 ]*:	00 00 00 00 	\.long 0x0
[0-9a-f	 ]*:	(4b ff ff e4|e4 ff ff 4b) 	b       .* <_start>
	\.\.\.

[0-9a-f	 ]*<.*plt_branch.*>:
[0-9a-f	 ]*:	(e9 82 80 e8|e8 80 82 e9) 	ld      r12,-32536\(r2\)
[0-9a-f	 ]*: R_PPC64_TOC16_DS	\*ABS\*\+0x157f00e8
[0-9a-f	 ]*:	(7d 89 03 a6|a6 03 89 7d) 	mtctr   r12
[0-9a-f	 ]*:	(4e 80 04 20|20 04 80 4e) 	bctr

[0-9a-f	 ]*<.*long_branch.*>:
[0-9a-f	 ]*:	(49 bf 00 10|10 00 bf 49) 	b       .* <far>
[0-9a-f	 ]*: R_PPC64_REL24	\*ABS\*\+0x137e00fc

[0-9a-f	 ]*<.*plt_branch.*>:
[0-9a-f	 ]*:	(e9 82 80 f0|f0 80 82 e9) 	ld      r12,-32528\(r2\)
[0-9a-f	 ]*: R_PPC64_TOC16_DS	\*ABS\*\+0x157f00f0
[0-9a-f	 ]*:	(7d 89 03 a6|a6 03 89 7d) 	mtctr   r12
[0-9a-f	 ]*:	(4e 80 04 20|20 04 80 4e) 	bctr
	\.\.\.

0*137e00fc <far>:
[0-9a-f	 ]*:	(4e 80 00 20|20 00 80 4e) 	blr
	\.\.\.

0*13bf00e0 <far2far>:
[0-9a-f	 ]*:	(4e 80 00 20|20 00 80 4e) 	blr
	\.\.\.

0*157e00e4 <huge>:
[0-9a-f	 ]*:	(4e 80 00 20|20 00 80 4e) 	blr

Disassembly of section \.branch_lt:

0*157f00e8 .*:
[0-9a-f	 ]*:	(00 00 00 00|e0 00 bf 13) .*
[0-9a-f	 ]*: R_PPC64_RELATIVE	\*ABS\*\+0x13bf00e0
[0-9a-f	 ]*:	(13 bf 00 e0|00 00 00 00) .*
[0-9a-f	 ]*:	(00 00 00 00|e4 00 7e 15) .*
[0-9a-f	 ]*: R_PPC64_RELATIVE	\*ABS\*\+0x157e00e4
[0-9a-f	 ]*:	(15 7e 00 e4|00 00 00 00) .*
