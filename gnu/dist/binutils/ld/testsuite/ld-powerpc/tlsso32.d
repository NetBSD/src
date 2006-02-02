#source: tls32.s
#as: -a32
#ld: -shared -melf32ppc
#objdump: -dr
#target: powerpc*-*-*

.*: +file format elf32-powerpc

Disassembly of section \.text:

.* <_start>:
.*:	38 7f 00 1c 	addi    r3,r31,28
.*:	48 00 00 01 	bl      .*
.*:	38 7f 00 0c 	addi    r3,r31,12
.*:	48 00 00 01 	bl      .*
.*:	38 7f 00 24 	addi    r3,r31,36
.*:	48 01 01 95 	bl      .*<__tls_get_addr@plt>
.*:	38 7f 00 0c 	addi    r3,r31,12
.*:	48 01 01 8d 	bl      .*<__tls_get_addr@plt>
.*:	39 23 80 20 	addi    r9,r3,-32736
.*:	3d 23 00 00 	addis   r9,r3,0
.*:	81 49 80 24 	lwz     r10,-32732\(r9\)
.*:	81 3f 00 2c 	lwz     r9,44\(r31\)
.*:	7d 49 12 2e 	lhzx    r10,r9,r2
.*:	89 42 00 00 	lbz     r10,0\(r2\)
.*:	3d 22 00 00 	addis   r9,r2,0
.*:	99 49 00 00 	stb     r10,0\(r9\)
.*:	38 7e 00 14 	addi    r3,r30,20
.*:	48 00 00 01 	bl      .*
.*:	38 7e 00 0c 	addi    r3,r30,12
.*:	48 00 00 01 	bl      .*
.*:	91 43 80 04 	stw     r10,-32764\(r3\)
.*:	3d 23 00 00 	addis   r9,r3,0
.*:	91 49 80 08 	stw     r10,-32760\(r9\)
.*:	81 3e 00 2c 	lwz     r9,44\(r30\)
.*:	7d 49 13 2e 	sthx    r10,r9,r2
.*:	a1 42 00 00 	lhz     r10,0\(r2\)
.*:	3d 22 00 00 	addis   r9,r2,0
.*:	a9 49 00 00 	lha     r10,0\(r9\)
Disassembly of section \.got:

.* <\.got>:
.*:	4e 80 00 21 	blrl
.*:	00 01 04 e4 	.*
	\.\.\.
