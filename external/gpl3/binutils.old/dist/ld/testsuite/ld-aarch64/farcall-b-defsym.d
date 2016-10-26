#name: aarch64-farcall-b-defsym
#source: farcall-b-defsym.s
#as:
#ld: -Ttext 0x1000 --defsym=bar=0x8001000
#objdump: -dr
#...

Disassembly of section .text:

0000000000001000 <_start>:
 +1000:	14000003 	b	100c <__bar_veneer>
 +1004:	d65f03c0 	ret
[ \t]+1008:[ \t]+14000007[ \t]+b[ \t]+1024 <__bar_veneer\+0x18>
000000000000100c <__bar_veneer>:
    100c:	90040010 	adrp	x16, 8001000 <bar>
    1010:	91000210 	add	x16, x16, #0x0
    1014:	d61f0200 	br	x16
	...
