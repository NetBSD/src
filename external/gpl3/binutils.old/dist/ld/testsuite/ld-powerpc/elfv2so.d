#source: elfv2.s
#as: -a64
#ld: -melf64ppc -shared
#objdump: -dr

.*

Disassembly of section \.text:

0+300 <.*\.plt_call\.f4>:
.*:	(f8 41 00 18|18 00 41 f8) 	std     r2,24\(r1\)
.*:	(e9 82 80 38|38 80 82 e9) 	ld      r12,-32712\(r2\)
.*:	(7d 89 03 a6|a6 03 89 7d) 	mtctr   r12
.*:	(4e 80 04 20|20 04 80 4e) 	bctr

0+310 <.*\.plt_call\.f3>:
.*:	(f8 41 00 18|18 00 41 f8) 	std     r2,24\(r1\)
.*:	(e9 82 80 28|28 80 82 e9) 	ld      r12,-32728\(r2\)
.*:	(7d 89 03 a6|a6 03 89 7d) 	mtctr   r12
.*:	(4e 80 04 20|20 04 80 4e) 	bctr

0+320 <.*\.plt_call\.f2>:
.*:	(f8 41 00 18|18 00 41 f8) 	std     r2,24\(r1\)
.*:	(e9 82 80 30|30 80 82 e9) 	ld      r12,-32720\(r2\)
.*:	(7d 89 03 a6|a6 03 89 7d) 	mtctr   r12
.*:	(4e 80 04 20|20 04 80 4e) 	bctr

0+330 <.*\.plt_call\.f1>:
.*:	(f8 41 00 18|18 00 41 f8) 	std     r2,24\(r1\)
.*:	(e9 82 80 40|40 80 82 e9) 	ld      r12,-32704\(r2\)
.*:	(7d 89 03 a6|a6 03 89 7d) 	mtctr   r12
.*:	(4e 80 04 20|20 04 80 4e) 	bctr

0+340 <f1>:
.*:	(3c 4c 00 02|02 00 4c 3c) 	addis   r2,r12,2
.*:	(38 42 82 c0|c0 82 42 38) 	addi    r2,r2,-32064
.*:	(7c 08 02 a6|a6 02 08 7c) 	mflr    r0
.*:	(f8 21 ff e1|e1 ff 21 f8) 	stdu    r1,-32\(r1\)
.*:	(f8 01 00 30|30 00 01 f8) 	std     r0,48\(r1\)
.*:	(4b ff ff dd|dd ff ff 4b) 	bl      .*\.plt_call\.f1>
.*:	(e8 62 80 08|08 80 62 e8) 	ld      r3,-32760\(r2\)
.*:	(4b ff ff c5|c5 ff ff 4b) 	bl      .*\.plt_call\.f2>
.*:	(e8 41 00 18|18 00 41 e8) 	ld      r2,24\(r1\)
.*:	(e8 62 80 10|10 80 62 e8) 	ld      r3,-32752\(r2\)
.*:	(4b ff ff a9|a9 ff ff 4b) 	bl      .*\.plt_call\.f3>
.*:	(e8 41 00 18|18 00 41 e8) 	ld      r2,24\(r1\)
.*:	(4b ff ff 91|91 ff ff 4b) 	bl      .*\.plt_call\.f4>
.*:	(e8 41 00 18|18 00 41 e8) 	ld      r2,24\(r1\)
.*:	(e8 01 00 30|30 00 01 e8) 	ld      r0,48\(r1\)
.*:	(38 21 00 20|20 00 21 38) 	addi    r1,r1,32
.*:	(7c 08 03 a6|a6 03 08 7c) 	mtlr    r0
.*:	(4e 80 00 20|20 00 80 4e) 	blr
.*:	(00 00 00 00|80 02 01 00) 	.*
.*:	(00 01 02 80|00 00 00 00) 	.*

0+390 <__glink_PLTresolve>:
.*:	(7c 08 02 a6|a6 02 08 7c) 	mflr    r0
.*:	(42 9f 00 05|05 00 9f 42) 	bcl     .*
.*:	(7d 68 02 a6|a6 02 68 7d) 	mflr    r11
.*:	(e8 4b ff f0|f0 ff 4b e8) 	ld      r2,-16\(r11\)
.*:	(7c 08 03 a6|a6 03 08 7c) 	mtlr    r0
.*:	(7d 8b 60 50|50 60 8b 7d) 	subf    r12,r11,r12
.*:	(7d 62 5a 14|14 5a 62 7d) 	add     r11,r2,r11
.*:	(38 0c ff d0|d0 ff 0c 38) 	addi    r0,r12,-48
.*:	(e9 8b 00 00|00 00 8b e9) 	ld      r12,0\(r11\)
.*:	(78 00 f0 82|82 f0 00 78) 	rldicl  r0,r0,62,2
.*:	(7d 89 03 a6|a6 03 89 7d) 	mtctr   r12
.*:	(e9 6b 00 08|08 00 6b e9) 	ld      r11,8\(r11\)
.*:	(4e 80 04 20|20 04 80 4e) 	bctr
.*:	(60 00 00 00|00 00 00 60) 	nop

.* <f3@plt>:
.*:	(4b ff ff c8|c8 ff ff 4b) 	b       .* <__glink_PLTresolve>

.* <f2@plt>:
.*:	(4b ff ff c4|c4 ff ff 4b) 	b       .* <__glink_PLTresolve>

.* <f4@plt>:
.*:	(4b ff ff c0|c0 ff ff 4b) 	b       .* <__glink_PLTresolve>

.* <f1@plt>:
.*:	(4b ff ff bc|bc ff ff 4b) 	b       .* <__glink_PLTresolve>
