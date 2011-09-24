
.*:     file format .*

Disassembly of section .text:

00001000 <_start>:
    1000:	eb000004 	bl	1018 <__bar_from_arm>
    1004:	eb00000e 	bl	1044 <__bar2_veneer>
    1008:	eb000005 	bl	1024 <__bar3_veneer>
    100c:	eb000009 	bl	1038 <__bar4_from_arm>
    1010:	eb000005 	bl	102c <__bar5_from_arm>
    1014:	00000000 	andeq	r0, r0, r0

00001018 <__bar_from_arm>:
    1018:	e59fc000 	ldr	ip, \[pc, #0\]	; 1020 <__bar_from_arm\+0x8>
    101c:	e12fff1c 	bx	ip
    1020:	02002021 	.word	0x02002021
00001024 <__bar3_veneer>:
    1024:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1028 <__bar3_veneer\+0x4>
    1028:	02002028 	.word	0x02002028
0000102c <__bar5_from_arm>:
    102c:	e59fc000 	ldr	ip, \[pc, #0\]	; 1034 <__bar5_from_arm\+0x8>
    1030:	e12fff1c 	bx	ip
    1034:	0200202f 	.word	0x0200202f
00001038 <__bar4_from_arm>:
    1038:	e59fc000 	ldr	ip, \[pc, #0\]	; 1040 <__bar4_from_arm\+0x8>
    103c:	e12fff1c 	bx	ip
    1040:	0200202d 	.word	0x0200202d

00001044 <__bar2_veneer>:
    1044:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1048 <__bar2_veneer\+0x4>
    1048:	02002024 	.word	0x02002024
	...

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
