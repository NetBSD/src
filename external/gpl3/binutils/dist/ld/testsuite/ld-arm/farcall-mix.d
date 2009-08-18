
.*:     file format .*

Disassembly of section .text:

00001000 <__bar2_veneer>:
    1000:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1004 <__bar2_veneer\+0x4>
    1004:	02002024 	.word	0x02002024
00001008 <__bar_from_arm>:
    1008:	e59fc000 	ldr	ip, \[pc, #0\]	; 1010 <__bar_from_arm\+0x8>
    100c:	e12fff1c 	bx	ip
    1010:	02002021 	.word	0x02002021
00001014 <__bar3_veneer>:
    1014:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1018 <__bar3_veneer\+0x4>
    1018:	02002028 	.word	0x02002028
0000101c <__bar4_from_arm>:
    101c:	e59fc000 	ldr	ip, \[pc, #0\]	; 1024 <__bar4_from_arm\+0x8>
    1020:	e12fff1c 	bx	ip
    1024:	0200202d 	.word	0x0200202d
00001028 <__bar5_from_arm>:
    1028:	e59fc000 	ldr	ip, \[pc, #0\]	; 1030 <__bar5_from_arm\+0x8>
    102c:	e12fff1c 	bx	ip
    1030:	0200202f 	.word	0x0200202f
	...

00001040 <_start>:
    1040:	ebfffff0 	bl	1008 <__bar_from_arm>
    1044:	ebffffed 	bl	1000 <__bar2_veneer>
    1048:	ebfffff1 	bl	1014 <__bar3_veneer>
    104c:	ebfffff2 	bl	101c <__bar4_from_arm>
    1050:	ebfffff4 	bl	1028 <__bar5_from_arm>
Disassembly of section .foo:

02002020 <bar>:
 2002020:	4770      	bx	lr
	...

02002024 <bar2>:
 2002024:	e12fff1e 	bx	lr

02002028 <bar3>:
 2002028:	e12fff1e 	bx	lr

0200202c <bar4>:
 200202c:	4770      	bx	lr

0200202e <bar5>:
 200202e:	4770      	bx	lr
