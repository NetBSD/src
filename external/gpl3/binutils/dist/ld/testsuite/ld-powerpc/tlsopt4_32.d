#source: tlsopt4_32.s
#source: tlslib32.s
#as: -a32
#ld: -melf32ppc
#objdump: -dr
#target: powerpc*-*-*

.*: +file format elf32-powerpc

Disassembly of section \.text:

0+1800094 <__tls_get_addr>:
 1800094:	4e 80 00 20 	blr

Disassembly of section \.opt1:

0+1800098 <\.opt1>:
 1800098:	3c 62 00 00 	addis   r3,r2,0
 180009c:	2c 04 00 00 	cmpwi   r4,0
 18000a0:	41 82 00 0c 	beq-    .*
 18000a4:	38 63 90 10 	addi    r3,r3,-28656
 18000a8:	48 00 00 08 	b       .*
 18000ac:	38 63 90 10 	addi    r3,r3,-28656

Disassembly of section \.opt2:

0+18000b0 <\.opt2>:
 18000b0:	3c 62 00 00 	addis   r3,r2,0
 18000b4:	2c 04 00 00 	cmpwi   r4,0
 18000b8:	41 82 00 08 	beq-    .*
 18000bc:	3c 62 00 00 	addis   r3,r2,0
 18000c0:	38 63 90 10 	addi    r3,r3,-28656

Disassembly of section \.opt3:

0+18000c4 <\.opt3>:
 18000c4:	3c 62 00 00 	addis   r3,r2,0
 18000c8:	48 00 00 0c 	b       .*
 18000cc:	3c 62 00 00 	addis   r3,r2,0
 18000d0:	48 00 00 0c 	b       .*
 18000d4:	38 63 90 10 	addi    r3,r3,-28656
 18000d8:	48 00 00 08 	b       .*
 18000dc:	38 63 90 08 	addi    r3,r3,-28664
#pass
