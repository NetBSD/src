#name: aarch64-farcall-back
#source: farcall-back.s
#as:
#ld: -Ttext 0x1000 --section-start .foo=0x100000000
#notarget: aarch64_be-*-*
#objdump: -dr
#...

Disassembly of section .text:

0000000000001000 <_start>:
    1000:	14000412 	b	2048 <__bar1_veneer>
    1004:	94000411 	bl	2048 <__bar1_veneer>
    1008:	14000406 	b	2020 <__bar2_veneer>
    100c:	94000405 	bl	2020 <__bar2_veneer>
    1010:	14000408 	b	2030 <__bar3_veneer>
    1014:	94000407 	bl	2030 <__bar3_veneer>
    1018:	d65f03c0 	ret
	...

000000000000201c <_back>:
    201c:	d65f03c0 	ret

0000000000002020 <__bar2_veneer>:
    2020:	f07ffff0 	adrp	x16, 100001000 <bar1\+0x1000>
    2024:	91002210 	add	x16, x16, #0x8
    2028:	d61f0200 	br	x16
    202c:	00000000 	.inst	0x00000000 ; undefined

0000000000002030 <__bar3_veneer>:
    2030:	58000090 	ldr	x16, 2040 <__bar3_veneer\+0x10>
    2034:	10000011 	adr	x17, 2034 <__bar3_veneer\+0x4>
    2038:	8b110210 	add	x16, x16, x17
    203c:	d61f0200 	br	x16
    2040:	ffffffdc 	.word	0xffffffdc
    2044:	00000000 	.word	0x00000000

0000000000002048 <__bar1_veneer>:
    2048:	d07ffff0 	adrp	x16, 100000000 <bar1>
    204c:	91000210 	add	x16, x16, #0x0
    2050:	d61f0200 	br	x16
	...

Disassembly of section .foo:

0000000100000000 <bar1>:
   100000000:	d65f03c0 	ret
   100000004:	14000805 	b	100002018 <___start_veneer>
	...

0000000100001008 <bar2>:
   100001008:	d65f03c0 	ret
   10000100c:	14000403 	b	100002018 <___start_veneer>
	...

0000000100002010 <bar3>:
   100002010:	d65f03c0 	ret
   100002014:	14000007 	b	100002030 <___back_veneer>

0000000100002018 <___start_veneer>:
   100002018:	58000090 	ldr	x16, 100002028 <___start_veneer\+0x10>
   10000201c:	10000011 	adr	x17, 10000201c <___start_veneer\+0x4>
   100002020:	8b110210 	add	x16, x16, x17
   100002024:	d61f0200 	br	x16
   100002028:	ffffefe4 	.word	0xffffefe4
   10000202c:	fffffffe 	.word	0xfffffffe

0000000100002030 <___back_veneer>:
   100002030:	90800010 	adrp	x16, 2000 <_start\+0x1000>
   100002034:	91007210 	add	x16, x16, #0x1c
   100002038:	d61f0200 	br	x16
	...
