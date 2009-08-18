
.*:     file format .*

Disassembly of section .text:

00001000 <__bar2_veneer>:
    1000:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1004 <__bar2_veneer\+0x4>
    1004:	02003024 	.word	0x02003024
00001008 <__bar_from_arm>:
    1008:	e59fc000 	ldr	ip, \[pc, #0\]	; 1010 <__bar_from_arm\+0x8>
    100c:	e12fff1c 	bx	ip
    1010:	02003021 	.word	0x02003021
00001014 <__bar3_veneer>:
    1014:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1018 <__bar3_veneer\+0x4>
    1018:	02003028 	.word	0x02003028
0000101c <__bar4_from_arm>:
    101c:	e59fc000 	ldr	ip, \[pc, #0\]	; 1024 <__bar4_from_arm\+0x8>
    1020:	e12fff1c 	bx	ip
    1024:	0200302d 	.word	0x0200302d
00001028 <__bar5_from_arm>:
    1028:	e59fc000 	ldr	ip, \[pc, #0\]	; 1030 <__bar5_from_arm\+0x8>
    102c:	e12fff1c 	bx	ip
    1030:	0200302f 	.word	0x0200302f
	...

00001040 <_start>:
    1040:	ebfffff0 	bl	1008 <__bar_from_arm>
    1044:	ebffffed 	bl	1000 <__bar2_veneer>

00001048 <myfunc>:
    1048:	ebfffff1 	bl	1014 <__bar3_veneer>
    104c:	ebfffff2 	bl	101c <__bar4_from_arm>
    1050:	ebfffff4 	bl	1028 <__bar5_from_arm>
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
