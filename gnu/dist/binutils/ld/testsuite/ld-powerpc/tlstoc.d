#source: tlslib.s
#source: tlstoc.s
#as: -a64
#ld: -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

00000000100000e8 <\.__tls_get_addr>:
    100000e8:	4e 80 00 20 	blr

00000000100000ec <_start>:
    100000ec:	3c 6d 00 00 	addis	r3,r13,0
    100000f0:	60 00 00 00 	nop
    100000f4:	38 63 90 58 	addi	r3,r3,-28584
    100000f8:	3c 6d 00 00 	addis	r3,r13,0
    100000fc:	60 00 00 00 	nop
    10000100:	38 63 10 00 	addi	r3,r3,4096
    10000104:	3c 6d 00 00 	addis	r3,r13,0
    10000108:	60 00 00 00 	nop
    1000010c:	38 63 90 58 	addi	r3,r3,-28584
    10000110:	3c 6d 00 00 	addis	r3,r13,0
    10000114:	60 00 00 00 	nop
    10000118:	38 63 10 00 	addi	r3,r3,4096
    1000011c:	39 23 80 50 	addi	r9,r3,-32688
    10000120:	3d 23 00 00 	addis	r9,r3,0
    10000124:	81 49 80 58 	lwz	r10,-32680\(r9\)
    10000128:	3d 2d 00 00 	addis	r9,r13,0
    1000012c:	7d 49 18 2a 	ldx	r10,r9,r3
    10000130:	3d 2d 00 00 	addis	r9,r13,0
    10000134:	a1 49 90 a0 	lhz	r10,-28512\(r9\)
    10000138:	89 4d 90 70 	lbz	r10,-28560\(r13\)
    1000013c:	3d 2d 00 00 	addis	r9,r13,0
    10000140:	99 49 90 78 	stb	r10,-28552\(r9\)
