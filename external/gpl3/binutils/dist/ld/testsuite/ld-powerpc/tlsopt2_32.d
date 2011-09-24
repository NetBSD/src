#source: tlsopt2_32.s
#source: tlslib32.s
#as: -a32
#ld: -melf32ppc
#objdump: -dr
#target: powerpc*-*-*

.*: +file format elf32-powerpc

Disassembly of section \.text:

0+1800094 <__tls_get_addr>:
 1800094:	4e 80 00 20 	blr

Disassembly of section \.no_opt2:

0+1800098 <\.no_opt2>:
 1800098:	38 6d ff f4 	addi    r3,r13,-12
 180009c:	2c 04 00 00 	cmpwi   r4,0
 18000a0:	41 82 00 08 	beq-    .*
 18000a4:	38 6d ff f4 	addi    r3,r13,-12
 18000a8:	4b ff ff ed 	bl      1800094 <__tls_get_addr>
#pass
