
.*


Disassembly of section \.plt:

00008000 <\.plt>:
    8000:	e52de004 	push	{lr}		; \(str lr, \[sp, #-4\]!\)
    8004:	e59fe004 	ldr	lr, \[pc, #4\]	; 8010 <foo-0xfe0>
    8008:	e08fe00e 	add	lr, pc, lr
    800c:	e5bef008 	ldr	pc, \[lr, #8\]!
    8010:	00000ffc 	\.word	0x00000ffc
    8014:	e28fc600 	add	ip, pc, #0
    8018:	e28cca00 	add	ip, ip, #0
    801c:	e5bcfffc 	ldr	pc, \[ip, #4092\]!	; 0xffc

Disassembly of section \.text:

00008ff0 <foo>:
    8ff0:	46c0      	nop			; \(mov r8, r8\)
    8ff2:	f240 0000 	movw	r0, #0
    8ff6:	f240 0000 	movw	r0, #0
    8ffa:	f240 0000 	movw	r0, #0
    8ffe:	f000 e804 	blx	9008 <foo\+0x18>
    9002:	0000      	movs	r0, r0
    9004:	0000      	movs	r0, r0
    9006:	0000      	movs	r0, r0
    9008:	eafffc01 	b	8014 <foo-0xfdc>
