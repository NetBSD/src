
.*:     file format .*

Disassembly of section .text:

00001000 <__bar2_veneer>:
    1000:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1004 <__bar2_veneer\+0x4>
    1004:	02003024 	.word	0x02003024
00001008 <__bar_from_arm>:
    1008:	e59fc000 	ldr	ip, \[pc, #0\]	; 1010 <__bar_from_arm\+0x8>
    100c:	e12fff1c 	bx	ip
    1010:	02003021 	.word	0x02003021
    1014:	00000000 	.word	0x00000000

00001018 <_start>:
    1018:	ebfffffa 	bl	1008 <__bar_from_arm>
    101c:	ebfffff7 	bl	1000 <__bar2_veneer>
Disassembly of section .mytext:

00002000 <__bar5_from_arm>:
    2000:	e59fc000 	ldr	ip, \[pc, #0\]	; 2008 <__bar5_from_arm\+0x8>
    2004:	e12fff1c 	bx	ip
    2008:	0200302f 	.word	0x0200302f
0000200c <__bar3_veneer>:
    200c:	e51ff004 	ldr	pc, \[pc, #-4\]	; 2010 <__bar3_veneer\+0x4>
    2010:	02003028 	.word	0x02003028
00002014 <__bar4_from_arm>:
    2014:	e59fc000 	ldr	ip, \[pc, #0\]	; 201c <__bar4_from_arm\+0x8>
    2018:	e12fff1c 	bx	ip
    201c:	0200302d 	.word	0x0200302d
	...
    2028:	ebfffff7 	bl	200c <__bar3_veneer>
    202c:	ebfffff8 	bl	2014 <__bar4_from_arm>
    2030:	ebfffff2 	bl	2000 <__bar5_from_arm>
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
