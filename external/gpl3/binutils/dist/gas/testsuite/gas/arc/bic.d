#as: -mcpu=arc700
#objdump: -dr --prefix-addresses --show-raw-insn

.*: +file format .*arc.*

Disassembly of section .text:
0x[0-9a-f]+ 2106 0080           	bic	r0,r1,r2
0x[0-9a-f]+ 2306 371a           	bic	gp,fp,sp
0x[0-9a-f]+ 2606 37dd           	bic	ilink,r30,blink
0x[0-9a-f]+ 2146 0000           	bic	r0,r1,0
0x[0-9a-f]+ 2606 7080 0000 0000 	bic	r0,0,r2
0x[0-9a-f]+ 2106 00be           	bic	0,r1,r2
0x[0-9a-f]+ 2106 0f80 ffff ffff 	bic	r0,r1,0xffffffff
0x[0-9a-f]+ 2606 7080 ffff ffff 	bic	r0,0xffffffff,r2
0x[0-9a-f]+ 2106 0f80 0000 00ff 	bic	r0,r1,0xff
0x[0-9a-f]+ 2606 7080 0000 00ff 	bic	r0,0xff,r2
0x[0-9a-f]+ 2106 0f80 ffff ff00 	bic	r0,r1,0xffffff00
0x[0-9a-f]+ 2606 7080 ffff ff00 	bic	r0,0xffffff00,r2
0x[0-9a-f]+ 2106 0f80 0000 0100 	bic	r0,r1,0x100
0x[0-9a-f]+ 2606 7080 ffff feff 	bic	r0,0xfffffeff,r2
0x[0-9a-f]+ 2606 7f80 0000 0100 	bic	r0,0x100,0x100
0x[0-9a-f]+ 2106 0f80 0000 0000 	bic	r0,r1,0
			68: ARC_32_ME	foo
0x[0-9a-f]+ 20c6 0080           	bic	r0,r0,r2
0x[0-9a-f]+ 23c6 0140           	bic	r3,r3,r5
0x[0-9a-f]+ 26c6 0201           	biceq	r6,r6,r8
0x[0-9a-f]+ 21c6 12c1           	biceq	r9,r9,r11
0x[0-9a-f]+ 24c6 1382           	bicne	r12,r12,r14
0x[0-9a-f]+ 27c6 1442           	bicne	r15,r15,r17
0x[0-9a-f]+ 22c6 2503           	bicp	r18,r18,r20
0x[0-9a-f]+ 25c6 25c3           	bicp	r21,r21,r23
0x[0-9a-f]+ 20c6 3684           	bicn	r24,r24,gp
0x[0-9a-f]+ 23c6 3744           	bicn	fp,fp,ilink
0x[0-9a-f]+ 26c6 37c5           	bicc	r30,r30,blink
0x[0-9a-f]+ 23c6 00c5           	bicc	r3,r3,r3
0x[0-9a-f]+ 23c6 0205           	bicc	r3,r3,r8
0x[0-9a-f]+ 23c6 0106           	bicnc	r3,r3,r4
0x[0-9a-f]+ 24c6 0106           	bicnc	r4,r4,r4
0x[0-9a-f]+ 24c6 01c6           	bicnc	r4,r4,r7
0x[0-9a-f]+ 24c6 0147           	bicv	r4,r4,r5
0x[0-9a-f]+ 25c6 0147           	bicv	r5,r5,r5
0x[0-9a-f]+ 25c6 0148           	bicnv	r5,r5,r5
0x[0-9a-f]+ 25c6 0148           	bicnv	r5,r5,r5
0x[0-9a-f]+ 26c6 0009           	bicgt	r6,r6,r0
0x[0-9a-f]+ 20c6 002a           	bicge	r0,r0,0
0x[0-9a-f]+ 21c6 006b           	biclt	r1,r1,0x1
0x[0-9a-f]+ 23c6 00ed           	bichi	r3,r3,0x3
0x[0-9a-f]+ 24c6 012e           	bicls	r4,r4,0x4
0x[0-9a-f]+ 25c6 016f           	bicpnz	r5,r5,0x5
0x[0-9a-f]+ 2106 8080           	bic.f	r0,r1,r2
0x[0-9a-f]+ 2146 8040           	bic.f	r0,r1,0x1
0x[0-9a-f]+ 2606 f080 0000 0001 	bic.f	r0,0x1,r2
0x[0-9a-f]+ 2106 80be           	bic.f	0,r1,r2
0x[0-9a-f]+ 2106 8f80 0000 0200 	bic.f	r0,r1,0x200
0x[0-9a-f]+ 2606 f080 0000 0200 	bic.f	r0,0x200,r2
0x[0-9a-f]+ 21c6 8081           	bic.feq	r1,r1,r2
0x[0-9a-f]+ 20c6 8022           	bic.fne	r0,r0,0
0x[0-9a-f]+ 22c6 808b           	bic.flt	r2,r2,r2
0x[0-9a-f]+ 26c6 f0a9 0000 0001 	bic.fgt	0,0x1,0x2
0x[0-9a-f]+ 26c6 ff8c 0000 0200 	bic.fle	0,0x200,0x200
0x[0-9a-f]+ 26c6 f0aa 0000 0200 	bic.fge	0,0x200,0x2
