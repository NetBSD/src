#name: aarch64-farcall-bl-none-function
#source: farcall-bl-none-function.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x8001000
#objdump: -dr
#...

Disassembly of section .text:

.* <_start>:
    1000:	94000003 	bl	100c <__bar_veneer>
    1004:	d65f03c0 	ret
    1008:	14000007 	b	1024 <__bar_veneer\+0x18>

.* <__bar_veneer>:
    100c:	90040010 	adrp	x16, 8001000 <bar>
    1010:	91000210 	add	x16, x16, #0x0
    1014:	d61f0200 	br	x16
	...

Disassembly of section .foo:

.* <bar>:
 8001000:	d65f03c0 	ret
