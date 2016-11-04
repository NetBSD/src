#name: aarch64-farcall-bl-section
#source: farcall-bl-section.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x8001000
#objdump: -dr
#...

Disassembly of section .text:

.* <_start>:
    1000:	94000008 	bl	1020 <___veneer>
    1004:	94000003 	bl	1010 <___veneer>
    1008:	d65f03c0 	ret
    100c:	1400000d 	b	1040 <___veneer\+0x20>

.* <___veneer>:
    1010:	90040010 	adrp	x16, 8001000 <bar>
    1014:	91001210 	add	x16, x16, #0x4
    1018:	d61f0200 	br	x16
    101c:	00000000 	.inst	0x00000000 ; undefined

.* <___veneer>:
    1020:	90040010 	adrp	x16, 8001000 <bar>
    1024:	91000210 	add	x16, x16, #0x0
    1028:	d61f0200 	br	x16
	...

Disassembly of section .foo:

.* <bar>:
 8001000:	d65f03c0 	ret

.* <bar2>:
 8001004:	d65f03c0 	ret
