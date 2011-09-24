#source: tlsopt1.s
#source: tlslib.s
#as: -a64
#ld: -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

0+100000e8 <\.__tls_get_addr>:
    100000e8:	4e 80 00 20 	blr

Disassembly of section \.no_opt1:

0+100000ec <\.no_opt1>:
    100000ec:	38 62 80 08 	addi    r3,r2,-32760
    100000f0:	2c 24 00 00 	cmpdi   r4,0
    100000f4:	41 82 00 10 	beq-    .*
    100000f8:	4b ff ff f1 	bl      100000e8 <\.__tls_get_addr>
    100000fc:	60 00 00 00 	nop
    10000100:	48 00 00 0c 	b       .*
    10000104:	4b ff ff e5 	bl      100000e8 <\.__tls_get_addr>
    10000108:	60 00 00 00 	nop
