#source: tls-tiny-gd-ie.s
#ld: -T relocs.ld -e0
#objdump: -dr
#...

Disassembly of section .text:

0000000000010000 \<test\>:
 +10000:	58080040 	ldr	x0, 20008 \<_GLOBAL_OFFSET_TABLE_\+0x8\>
 +10004:	d53bd041 	mrs	x1, tpidr_el0
 +10008:	8b000020 	add	x0, x1, x0
