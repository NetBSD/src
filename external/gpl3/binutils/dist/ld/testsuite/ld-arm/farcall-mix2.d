
.*:     file format .*

Disassembly of section .text:

00001000 <_start>:
    1000:	eb000000 	bl	1008 <__bar_from_arm>
    1004:	eb000002 	bl	1014 <__bar2_veneer>

00001008 <__bar_from_arm>:
    1008:	e59fc000 	ldr	ip, \[pc, #0\]	; 1010 <__bar_from_arm\+0x8>
    100c:	e12fff1c 	bx	ip
    1010:	02003021 	.word	0x02003021
00001014 <__bar2_veneer>:
    1014:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1018 <__bar2_veneer\+0x4>
    1018:	02003024 	.word	0x02003024
    101c:	00000000 	.word	0x00000000
Disassembly of section .mytext:

00002000 <__bar3_veneer-0x10>:
    2000:	eb000002 	bl	2010 <__bar3_veneer>
    2004:	eb000003 	bl	2018 <__bar4_from_arm>
    2008:	eb000005 	bl	2024 <__bar5_from_arm>
    200c:	00000000 	andeq	r0, r0, r0

00002010 <__bar3_veneer>:
    2010:	e51ff004 	ldr	pc, \[pc, #-4\]	; 2014 <__bar3_veneer\+0x4>
    2014:	02003028 	.word	0x02003028

00002018 <__bar4_from_arm>:
    2018:	e59fc000 	ldr	ip, \[pc, #0\]	; 2020 <__bar4_from_arm\+0x8>
    201c:	e12fff1c 	bx	ip
    2020:	0200302d 	.word	0x0200302d

00002024 <__bar5_from_arm>:
    2024:	e59fc000 	ldr	ip, \[pc, #0\]	; 202c <__bar5_from_arm\+0x8>
    2028:	e12fff1c 	bx	ip
    202c:	0200302f 	.word	0x0200302f
	...
Disassembly of section .foo:

02003020 <bar>:
 2003020:	4770      	bx	lr
	...

02003024 <bar2>:
 2003024:	e12fff1e 	bx	lr

02003028 <bar3>:
 2003028:	e12fff1e 	bx	lr

0200302c <bar4>:
 200302c:	4770      	bx	lr

0200302e <bar5>:
 200302e:	4770      	bx	lr
