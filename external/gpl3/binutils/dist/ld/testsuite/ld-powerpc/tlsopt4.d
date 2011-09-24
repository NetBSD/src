#source: tlsopt4.s
#source: tlslib.s
#as: -a64
#ld: -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

0+100000e8 <\.__tls_get_addr>:
    100000e8:	4e 80 00 20 	blr

Disassembly of section \.opt1:

0+100000ec <\.opt1>:
    100000ec:	3c 6d 00 00 	addis   r3,r13,0
    100000f0:	2c 24 00 00 	cmpdi   r4,0
    100000f4:	41 82 00 10 	beq-    .*
    100000f8:	60 00 00 00 	nop
    100000fc:	38 63 90 10 	addi    r3,r3,-28656
    10000100:	48 00 00 0c 	b       .*
    10000104:	60 00 00 00 	nop
    10000108:	38 63 90 10 	addi    r3,r3,-28656

Disassembly of section \.opt2:

0+1000010c <\.opt2>:
    1000010c:	3c 6d 00 00 	addis   r3,r13,0
    10000110:	2c 24 00 00 	cmpdi   r4,0
    10000114:	41 82 00 08 	beq-    .*
    10000118:	3c 6d 00 00 	addis   r3,r13,0
    1000011c:	60 00 00 00 	nop
    10000120:	38 63 90 10 	addi    r3,r3,-28656

Disassembly of section \.opt3:

0+10000124 <\.opt3>:
    10000124:	3c 6d 00 00 	addis   r3,r13,0
    10000128:	48 00 00 0c 	b       .*
    1000012c:	3c 6d 00 00 	addis   r3,r13,0
    10000130:	48 00 00 10 	b       .*
    10000134:	60 00 00 00 	nop
    10000138:	38 63 90 10 	addi    r3,r3,-28656
    1000013c:	48 00 00 0c 	b       .*
    10000140:	60 00 00 00 	nop
    10000144:	38 63 90 08 	addi    r3,r3,-28664
