
.*:     file format .*

Disassembly of section .text:

00001000 <_start>:
    1000:	eb000009 	bl	102c <__bar_from_arm>
    1004:	eb000006 	bl	1024 <__bar2_veneer>

00001008 <myfunc>:
    1008:	eb00000d 	bl	1044 <__bar3_veneer>
    100c:	eb000001 	bl	1018 <__bar4_from_arm>
    1010:	eb000008 	bl	1038 <__bar5_from_arm>
    1014:	00000000 	andeq	r0, r0, r0

00001018 <__bar4_from_arm>:
    1018:	e59fc000 	ldr	ip, \[pc, #0\]	; 1020 <__bar4_from_arm\+0x8>
    101c:	e12fff1c 	bx	ip
    1020:	0200302d 	.word	0x0200302d

00001024 <__bar2_veneer>:
    1024:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1028 <__bar2_veneer\+0x4>
    1028:	02003024 	.word	0x02003024

0000102c <__bar_from_arm>:
    102c:	e59fc000 	ldr	ip, \[pc, #0\]	; 1034 <__bar_from_arm\+0x8>
    1030:	e12fff1c 	bx	ip
    1034:	02003021 	.word	0x02003021

00001038 <__bar5_from_arm>:
    1038:	e59fc000 	ldr	ip, \[pc, #0\]	; 1040 <__bar5_from_arm\+0x8>
    103c:	e12fff1c 	bx	ip
    1040:	0200302f 	.word	0x0200302f

00001044 <__bar3_veneer>:
    1044:	e51ff004 	ldr	pc, \[pc, #-4\]	; 1048 <__bar3_veneer\+0x4>
    1048:	02003028 	.word	0x02003028
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
