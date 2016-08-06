#source: tlsopt5.s
#as: -a64
#ld: --gc-sections tlsdll.so
#objdump: -dr
#target: powerpc64*-*-*

.*

Disassembly of section \.text:

0000000010000300 <.*\.plt_call\.__tls_get_addr_opt@@GLIBC_2\.22>:
.*:	(00 00 63 e9|e9 63 00 00) 	ld      r11,0\(r3\)
.*:	(08 00 83 e9|e9 83 00 08) 	ld      r12,8\(r3\)
.*:	(78 1b 60 7c|7c 60 1b 78) 	mr      r0,r3
.*:	(00 00 2b 2c|2c 2b 00 00) 	cmpdi   r11,0
.*:	(14 6a 6c 7c|7c 6c 6a 14) 	add     r3,r12,r13
.*:	(20 00 82 4d|4d 82 00 20) 	beqlr   
.*:	(78 03 03 7c|7c 03 03 78) 	mr      r3,r0
.*:	(a6 02 68 7d|7d 68 02 a6) 	mflr    r11
.*:	(08 00 61 f9|f9 61 00 08) 	std     r11,8\(r1\)
.*:	(18 00 41 f8|f8 41 00 18) 	std     r2,24\(r1\)
.*:	(28 80 82 e9|e9 82 80 28) 	ld      r12,-32728\(r2\)
.*:	(a6 03 89 7d|7d 89 03 a6) 	mtctr   r12
.*:	(21 04 80 4e|4e 80 04 21) 	bctrl
.*:	(18 00 41 e8|e8 41 00 18) 	ld      r2,24\(r1\)
.*:	(08 00 61 e9|e9 61 00 08) 	ld      r11,8\(r1\)
.*:	(a6 03 68 7d|7d 68 03 a6) 	mtlr    r11
.*:	(20 00 80 4e|4e 80 00 20) 	blr

0000000010000344 <_start>:
.*:	(08 80 62 38|38 62 80 08) 	addi    r3,r2,-32760
.*:	(b9 ff ff 4b|4b ff ff b9) 	bl      .*
.*:	(00 00 00 60|60 00 00 00) 	nop
.*:	(b8 02 01 00|00 00 00 00) 	.*
.*:	(00 00 00 00|00 01 02 b8) 	.*

0000000010000358 <__glink_PLTresolve>:
.*:	(a6 02 08 7c|7c 08 02 a6) 	mflr    r0
.*:	(05 00 9f 42|42 9f 00 05) 	bcl     .*
.*:	(a6 02 68 7d|7d 68 02 a6) 	mflr    r11
.*:	(f0 ff 4b e8|e8 4b ff f0) 	ld      r2,-16\(r11\)
.*:	(a6 03 08 7c|7c 08 03 a6) 	mtlr    r0
.*:	(50 60 8b 7d|7d 8b 60 50) 	subf    r12,r11,r12
.*:	(14 5a 62 7d|7d 62 5a 14) 	add     r11,r2,r11
.*:	(d0 ff 0c 38|38 0c ff d0) 	addi    r0,r12,-48
.*:	(00 00 8b e9|e9 8b 00 00) 	ld      r12,0\(r11\)
.*:	(82 f0 00 78|78 00 f0 82) 	rldicl  r0,r0,62,2
.*:	(a6 03 89 7d|7d 89 03 a6) 	mtctr   r12
.*:	(08 00 6b e9|e9 6b 00 08) 	ld      r11,8\(r11\)
.*:	(20 04 80 4e|4e 80 04 20) 	bctr
.*:	(00 00 00 60|60 00 00 00) 	nop

0000000010000390 <__tls_get_addr_opt@plt>:
.*:	(c8 ff ff 4b|4b ff ff c8) 	b       .*
