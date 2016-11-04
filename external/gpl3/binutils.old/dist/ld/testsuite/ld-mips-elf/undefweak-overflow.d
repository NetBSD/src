#name: undefined weak symbol overflow
#source: undefweak-overflow.s
#ld: -Ttext=0x20000000 -e start
#objdump: -dr --show-raw-insn
#...
0*20000000 <_ftext>:
 *20000000:	d85fffff 	beqzc	v0,20000000 <_ftext>
 *20000004:	00000000 	nop
 *20000008:	f85ffffd 	bnezc	v0,20000000 <_ftext>
 *2000000c:	ec4ffffd 	lwpc	v0,20000000 <_ftext>
 *20000010:	ec5bfffe 	ldpc	v0,20000000 <_ftext>
 *20000014:	cbfffffa 	bc	20000000 <_ftext>
 *20000018:	ec9ee000 	auipc	a0,0xe000
 *2000001c:	2484ffe8 	addiu	a0,a0,-24
 *20000020:	1000fff7 	b	20000000 <_ftext>
 *20000024:	00000000 	nop
 *20000028:	0411fff5 	bal	20000000 <_ftext>
 *2000002c:	3c...... 	lui	a0,0x....

0*20000030 <micro>:
 *20000030:	8e67      	beqz	a0,20000000 <_ftext>
 *20000032:	0c00      	nop
 *20000034:	cfe5      	b	20000000 <_ftext>
 *20000036:	0c00      	nop
 *20000038:	9400 ffe2 	b	20000000 <_ftext>
 *2000003c:	0c00      	nop
#pass
