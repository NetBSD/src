#source: tls32.s
#as: -a32
#ld: -shared
#objdump: -dr
#target: powerpc*-*-*

.*

Disassembly of section \.text:

.* <_start>:
.*:	(38 7f ff e0|e0 ff 7f 38) 	addi    r3,r31,-32
.*:	(48 00 00 01|01 00 00 48) 	bl      .*
.*:	(38 7f ff f4|f4 ff 7f 38) 	addi    r3,r31,-12
.*:	(48 00 00 01|01 00 00 48) 	bl      .*
.*:	(38 7f ff e8|e8 ff 7f 38) 	addi    r3,r31,-24
.*:	(48 01 01 95|95 01 01 48) 	bl      .*<__tls_get_addr@plt>
.*:	(38 7f ff f4|f4 ff 7f 38) 	addi    r3,r31,-12
.*:	(48 01 01 8d|8d 01 01 48) 	bl      .*<__tls_get_addr@plt>
.*:	(39 23 80 20|20 80 23 39) 	addi    r9,r3,-32736
.*:	(3d 23 00 00|00 00 23 3d) 	addis   r9,r3,0
.*:	(81 49 80 24|24 80 49 81) 	lwz     r10,-32732\(r9\)
.*:	(81 3f ff f0|f0 ff 3f 81) 	lwz     r9,-16\(r31\)
.*:	(7d 49 12 2e|2e 12 49 7d) 	lhzx    r10,r9,r2
.*:	(89 42 00 00|00 00 42 89) 	lbz     r10,0\(r2\)
.*:	(3d 22 00 00|00 00 22 3d) 	addis   r9,r2,0
.*:	(99 49 00 00|00 00 49 99) 	stb     r10,0\(r9\)
.*:	(38 7e ff d8|d8 ff 7e 38) 	addi    r3,r30,-40
.*:	(48 00 00 01|01 00 00 48) 	bl      .*
.*:	(38 7e ff f4|f4 ff 7e 38) 	addi    r3,r30,-12
.*:	(48 00 00 01|01 00 00 48) 	bl      .*
.*:	(91 43 80 04|04 80 43 91) 	stw     r10,-32764\(r3\)
.*:	(3d 23 00 00|00 00 23 3d) 	addis   r9,r3,0
.*:	(91 49 80 08|08 80 49 91) 	stw     r10,-32760\(r9\)
.*:	(81 3e ff f0|f0 ff 3e 81) 	lwz     r9,-16\(r30\)
.*:	(7d 49 13 2e|2e 13 49 7d) 	sthx    r10,r9,r2
.*:	(a1 42 00 00|00 00 42 a1) 	lhz     r10,0\(r2\)
.*:	(3d 22 00 00|00 00 22 3d) 	addis   r9,r2,0
.*:	(a9 49 00 00|00 00 49 a9) 	lha     r10,0\(r9\)
Disassembly of section \.got:

.* <_GLOBAL_OFFSET_TABLE_-0x28>:
#...
.*:	(4e 80 00 21|21 00 80 4e) 	blrl
.* <_GLOBAL_OFFSET_TABLE_>:
.*:	(00 01 03 ec|ec 03 01 00) .*
#pass
