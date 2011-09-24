#source: tlsopt2.s
#source: tlslib.s
#as: -a64
#ld: -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

0+100000e8 <\.__tls_get_addr>:
    100000e8:	4e 80 00 20 	blr

Disassembly of section \.no_opt2:

0+100000ec <\.no_opt2>:
    100000ec:	38 62 80 08 	addi    r3,r2,-32760
    100000f0:	2c 24 00 00 	cmpdi   r4,0
    100000f4:	41 82 00 08 	beq-    .*
    100000f8:	38 62 80 08 	addi    r3,r2,-32760
    100000fc:	4b ff ff ed 	bl      100000e8 <\.__tls_get_addr>
    10000100:	60 00 00 00 	nop
