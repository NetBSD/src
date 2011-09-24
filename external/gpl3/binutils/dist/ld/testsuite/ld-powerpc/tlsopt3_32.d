#source: tlsopt3_32.s
#source: tlslib32.s
#as: -a32
#ld: -melf32ppc
#objdump: -dr
#target: powerpc*-*-*

.*: +file format elf32-powerpc

Disassembly of section \.text:

0+1800094 <__tls_get_addr>:
 1800094:	4e 80 00 20 	blr

Disassembly of section \.no_opt3:

0+1800098 <\.no_opt3>:
 1800098:	38 6d ff ec 	addi    r3,r13,-20
 180009c:	48 00 00 0c 	b       .*
 18000a0:	38 6d ff f4 	addi    r3,r13,-12
 18000a4:	48 00 00 0c 	b       .*
 18000a8:	4b ff ff ed 	bl      1800094 <__tls_get_addr>
 18000ac:	48 00 00 08 	b       .*
 18000b0:	4b ff ff e5 	bl      1800094 <__tls_get_addr>
#pass
