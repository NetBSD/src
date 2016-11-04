.*:     file format elf32-.*arm
architecture: arm.*, flags 0x00000150:
HAS_SYMS, DYNAMIC, D_PAGED
start address 0x.*

Disassembly of section .plt:

00008164 <.plt>:
    8164:	e52de004 	push	{lr}		; .*
    8168:	e59fe004 	ldr	lr, \[pc, #4\]	; .*
    816c:	e08fe00e 	add	lr, pc, lr
    8170:	e5bef008 	ldr	pc, \[lr, #8\]!
    8174:	000080f0 	.word	0x000080f0
    8178:	e08e0000 	add	r0, lr, r0
    817c:	e5901004 	ldr	r1, \[r0, #4\]
    8180:	e12fff11 	bx	r1
    8184:	e52d2004 	push	{r2}		; .*
    8188:	e59f200c 	ldr	r2, \[pc, #12\]	; .*
    818c:	e59f100c 	ldr	r1, \[pc, #12\]	; .*
    8190:	e79f2002 	ldr	r2, \[pc, r2\]
    8194:	e081100f 	add	r1, r1, pc
    8198:	e12fff12 	bx	r2
    819c:	000080e8 	.word	0x000080e8
    81a0:	000080c8 	.word	0x000080c8

Disassembly of section .text:

000081a8 <text>:
    81a8:	e59f0004 	ldr	r0, \[pc, #4\]	; .*
    81ac:	ebfffff1 	bl	8178 .*
    81b0:	e1a00000 	nop			; .*
    81b4:	000080c0 	.word	0x000080c0
    81b8:	4801      	ldr	r0, \[pc, #4\]	; .*
    81ba:	f000 f805 	bl	81c8 .*
    81be:	46c0      	nop			; .*
    81c0:	000080b1 	.word	0x000080b1
    81c4:	00000000 	.word	0x00000000

000081c8 <__unnamed_veneer>:
    81c8:	4778      	bx	pc
    81ca:	46c0      	nop			; .*
    81cc:	e59f1000 	ldr	r1, \[pc\]	; .*
    81d0:	e081f00f 	add	pc, r1, pc
    81d4:	ffffffa0 	.word	0xffffffa0

Disassembly of section .foo:

04001000 <foo>:
 4001000:	e59f0004 	ldr	r0, \[pc, #4\]	; .*
 4001004:	eb000009 	bl	4001030 .*
 4001008:	e1a00000 	nop			; .*
 400100c:	fc00f268 	.word	0xfc00f268
 4001010:	e59f0004 	ldr	r0, \[pc, #4\]	; .*
 4001014:	eb000005 	bl	4001030 .*
 4001018:	e1a00000 	nop			; .*
 400101c:	fc00f260 	.word	0xfc00f260
 4001020:	4801      	ldr	r0, \[pc, #4\]	; .*
 4001022:	f000 f80b 	bl	400103c .*
 4001026:	46c0      	nop			; .*
 4001028:	fc00f249 	.word	0xfc00f249
 400102c:	00000000 	.word	0x00000000

04001030 <__unnamed_veneer>:
 4001030:	e59f1000 	ldr	r1, \[pc\]	; .*
 4001034:	e08ff001 	add	pc, pc, r1
 4001038:	fc00713c 	.word	0xfc00713c

0400103c <__unnamed_veneer>:
 400103c:	4778      	bx	pc
 400103e:	46c0      	nop			; .*
 4001040:	e59f1000 	ldr	r1, \[pc\]	; .*
 4001044:	e081f00f 	add	pc, r1, pc
 4001048:	fc00712c 	.word	0xfc00712c
 400104c:	00000000 	.word	0x00000000
