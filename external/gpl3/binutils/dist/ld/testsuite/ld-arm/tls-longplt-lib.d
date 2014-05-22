.*:     file format elf32-.*arm
architecture: arm, flags 0x00000150:
HAS_SYMS, DYNAMIC, D_PAGED
start address 0x.*

Disassembly of section .plt:

00008198 <.plt>:
    8198:	e52de004 	push	{lr}		; .*
    819c:	e59fe004 	ldr	lr, \[pc, #4\]	; .*
    81a0:	e08fe00e 	add	lr, pc, lr
    81a4:	e5bef008 	ldr	pc, \[lr, #8\]!
    81a8:	000080e0 	.word	0x000080e0
    81ac:	e08e0000 	add	r0, lr, r0
    81b0:	e5901004 	ldr	r1, \[r0, #4\]
    81b4:	e12fff11 	bx	r1
    81b8:	e52d2004 	push	{r2}		; .*
    81bc:	e59f200c 	ldr	r2, \[pc, #12\]	; .*
    81c0:	e59f100c 	ldr	r1, \[pc, #12\]	; .*
    81c4:	e79f2002 	ldr	r2, \[pc, r2\]
    81c8:	e081100f 	add	r1, r1, pc
    81cc:	e12fff12 	bx	r2
    81d0:	000080d8 	.word	0x000080d8
    81d4:	000080b8 	.word	0x000080b8

Disassembly of section .text:

000081d8 <text>:
    81d8:	e59f0004 	ldr	r0, \[pc, #4\]	; .*
    81dc:	fafffff2 	blx	81ac .*
    81e0:	e1a00000 	nop			; .*
    81e4:	000080b4 	.word	0x000080b4
    81e8:	4801      	ldr	r0, \[pc, #4\]	; .*
    81ea:	f7ff efe0 	blx	81ac <.*>
    81ee:	46c0      	nop			; .*
    81f0:	000080a5 	.word	0x000080a5

Disassembly of section .foo:

04001000 <foo>:
 4001000:	e59f0004 	ldr	r0, \[pc, #4\]	; .*
 4001004:	fa000009 	blx	4001030 .*
 4001008:	e1a00000 	nop			; .*
 400100c:	fc00f28c 	.word	0xfc00f28c
 4001010:	e59f0004 	ldr	r0, \[pc, #4\]	; .*
 4001014:	fa000005 	blx	4001030 .*
 4001018:	e1a00000 	nop			; .*
 400101c:	fc00f284 	.word	0xfc00f284
 4001020:	4801      	ldr	r0, \[pc, #4\]	; .*
 4001022:	f000 e806 	blx	4001030 .*
 4001026:	46c0      	nop			; .*
 4001028:	fc00f26d 	.word	0xfc00f26d
 400102c:	00000000 	.word	0x00000000

04001030 <__unnamed_veneer>:
 4001030:	e59f1000 	ldr	r1, \[pc\]	; .*
 4001034:	e08ff001 	add	pc, pc, r1
 4001038:	fc007170 	.word	0xfc007170
 400103c:	00000000 	.word	0x00000000
