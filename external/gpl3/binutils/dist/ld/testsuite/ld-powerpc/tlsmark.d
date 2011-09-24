#source: tlsmark.s
#source: tlslib.s
#as: -a64
#ld: -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

0+100000e8 <_start>:
    100000e8:	48 00 00 18 	b       10000100 <_start\+0x18>
    100000ec:	60 00 00 00 	nop
    100000f0:	38 63 90 00 	addi    r3,r3,-28672
    100000f4:	e8 83 00 00 	ld      r4,0\(r3\)
    100000f8:	3c 6d 00 00 	addis   r3,r13,0
    100000fc:	48 00 00 0c 	b       10000108 <_start\+0x20>
    10000100:	3c 6d 00 00 	addis   r3,r13,0
    10000104:	4b ff ff e8 	b       100000ec <_start\+0x4>
    10000108:	60 00 00 00 	nop
    1000010c:	38 63 10 00 	addi    r3,r3,4096
    10000110:	e8 83 80 00 	ld      r4,-32768\(r3\)
    10000114:	3c 6d 00 00 	addis   r3,r13,0
    10000118:	48 00 00 0c 	b       10000124 <_start\+0x3c>
    1000011c:	3c 6d 00 00 	addis   r3,r13,0
    10000120:	48 00 00 14 	b       10000134 <_start\+0x4c>
    10000124:	60 00 00 00 	nop
    10000128:	38 63 90 04 	addi    r3,r3,-28668
    1000012c:	e8 a3 00 00 	ld      r5,0\(r3\)
    10000130:	4b ff ff ec 	b       1000011c <_start\+0x34>
    10000134:	60 00 00 00 	nop
    10000138:	38 63 10 00 	addi    r3,r3,4096
    1000013c:	e8 a3 80 04 	ld      r5,-32764\(r3\)

0+10000140 <\.__tls_get_addr>:
    10000140:	4e 80 00 20 	blr
