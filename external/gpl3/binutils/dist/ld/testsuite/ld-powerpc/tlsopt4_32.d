#source: tlsopt4_32.s
#source: tlslib32.s
#as: -a32
#ld: 
#objdump: -dr
#target: powerpc*-*-*

.*

Disassembly of section \.text:

0+1800094 <__tls_get_addr>:
.*:	(4e 80 00 20|20 00 80 4e) 	blr

Disassembly of section \.opt1:

0+1800098 <\.opt1>:
.*:	(3c 62 00 00|00 00 62 3c) 	addis   r3,r2,0
.*:	(2c 04 00 00|00 00 04 2c) 	cmpwi   r4,0
.*:	(41 82 00 0c|0c 00 82 41) 	beq     .*
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656
.*:	(48 00 00 08|08 00 00 48) 	b       .*
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656

Disassembly of section \.opt2:

0+18000b0 <\.opt2>:
.*:	(3c 62 00 00|00 00 62 3c) 	addis   r3,r2,0
.*:	(2c 04 00 00|00 00 04 2c) 	cmpwi   r4,0
.*:	(41 82 00 08|08 00 82 41) 	beq     .*
.*:	(3c 62 00 00|00 00 62 3c) 	addis   r3,r2,0
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656

Disassembly of section \.opt3:

0+18000c4 <\.opt3>:
.*:	(3c 62 00 00|00 00 62 3c) 	addis   r3,r2,0
.*:	(48 00 00 0c|0c 00 00 48) 	b       .*
.*:	(3c 62 00 00|00 00 62 3c) 	addis   r3,r2,0
.*:	(48 00 00 0c|0c 00 00 48) 	b       .*
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656
.*:	(48 00 00 08|08 00 00 48) 	b       .*
.*:	(38 63 90 08|08 90 63 38) 	addi    r3,r3,-28664
#pass
