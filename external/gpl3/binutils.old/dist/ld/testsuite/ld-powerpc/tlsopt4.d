#source: tlsopt4.s
#source: tlslib.s
#as: -a64
#ld: 
#objdump: -dr
#target: powerpc64*-*-*

.*

Disassembly of section \.text:

0+100000e8 <\.__tls_get_addr>:
.*:	(4e 80 00 20|20 00 80 4e) 	blr

Disassembly of section \.opt1:

0+100000ec <\.opt1>:
.*:	(3c 6d 00 00|00 00 6d 3c) 	addis   r3,r13,0
.*:	(2c 24 00 00|00 00 24 2c) 	cmpdi   r4,0
.*:	(41 82 00 10|10 00 82 41) 	beq     .*
.*:	(60 00 00 00|00 00 00 60) 	nop
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656
.*:	(48 00 00 0c|0c 00 00 48) 	b       .*
.*:	(60 00 00 00|00 00 00 60) 	nop
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656

Disassembly of section \.opt2:

0+1000010c <\.opt2>:
.*:	(3c 6d 00 00|00 00 6d 3c) 	addis   r3,r13,0
.*:	(2c 24 00 00|00 00 24 2c) 	cmpdi   r4,0
.*:	(41 82 00 08|08 00 82 41) 	beq     .*
.*:	(3c 6d 00 00|00 00 6d 3c) 	addis   r3,r13,0
.*:	(60 00 00 00|00 00 00 60) 	nop
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656

Disassembly of section \.opt3:

0+10000124 <\.opt3>:
.*:	(3c 6d 00 00|00 00 6d 3c) 	addis   r3,r13,0
.*:	(48 00 00 0c|0c 00 00 48) 	b       .*
.*:	(3c 6d 00 00|00 00 6d 3c) 	addis   r3,r13,0
.*:	(48 00 00 10|10 00 00 48) 	b       .*
.*:	(60 00 00 00|00 00 00 60) 	nop
.*:	(38 63 90 10|10 90 63 38) 	addi    r3,r3,-28656
.*:	(48 00 00 0c|0c 00 00 48) 	b       .*
.*:	(60 00 00 00|00 00 00 60) 	nop
.*:	(38 63 90 08|08 90 63 38) 	addi    r3,r3,-28664
