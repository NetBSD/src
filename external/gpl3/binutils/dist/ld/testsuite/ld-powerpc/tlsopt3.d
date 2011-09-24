#source: tlsopt3.s
#source: tlslib.s
#as: -a64
#ld: -melf64ppc
#objdump: -dr
#target: powerpc64*-*-*

.*: +file format elf64-powerpc

Disassembly of section \.text:

00000000100000e8 <\.__tls_get_addr>:
    100000e8:	4e 80 00 20 	blr

Disassembly of section \.no_opt3:

00000000100000ec <\.no_opt3>:
    100000ec:	38 62 80 08 	addi    r3,r2,-32760
    100000f0:	48 00 00 0c 	b       .*
    100000f4:	38 62 80 18 	addi    r3,r2,-32744
    100000f8:	48 00 00 10 	b       .*
    100000fc:	4b ff ff ed 	bl      100000e8 <\.__tls_get_addr>
    10000100:	60 00 00 00 	nop
    10000104:	48 00 00 0c 	b       .*
    10000108:	4b ff ff e1 	bl      100000e8 <\.__tls_get_addr>
    1000010c:	60 00 00 00 	nop
