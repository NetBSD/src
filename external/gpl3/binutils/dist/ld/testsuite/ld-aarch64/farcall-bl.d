#name: aarch64-farcall-bl
#source: farcall-bl.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x8001000
#objdump: -dr
#...

Disassembly of section .text:

0000000000001000 <_start>:
 +1000:	94000002 	bl	1008 <__bar_veneer>
 +1004:	d65f03c0 	ret

0000000000001008 <__bar_veneer>:
    1008:	90040010 	adrp	x16, 8001000 <bar>
    100c:	91000210 	add	x16, x16, #0x0
    1010:	d61f0200 	br	x16
	...

Disassembly of section .foo:

0000000008001000 <bar>:
 8001000:	d65f03c0 	ret
