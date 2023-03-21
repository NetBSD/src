#as: --EL
#objdump: -dr
#name: eBPF ALU instructions

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	04 02 00 00 9a 02 00 00 	add32 %r2,0x29a
   8:	04 03 00 00 66 fd ff ff 	add32 %r3,-666
  10:	04 04 00 00 ef be ad 7e 	add32 %r4,0x7eadbeef
  18:	0c 65 00 00 00 00 00 00 	add32 %r5,%r6
  20:	14 02 00 00 9a 02 00 00 	sub32 %r2,0x29a
  28:	14 03 00 00 66 fd ff ff 	sub32 %r3,-666
  30:	14 04 00 00 ef be ad 7e 	sub32 %r4,0x7eadbeef
  38:	1c 65 00 00 00 00 00 00 	sub32 %r5,%r6
  40:	24 02 00 00 9a 02 00 00 	mul32 %r2,0x29a
  48:	24 03 00 00 66 fd ff ff 	mul32 %r3,-666
  50:	24 04 00 00 ef be ad 7e 	mul32 %r4,0x7eadbeef
  58:	2c 65 00 00 00 00 00 00 	mul32 %r5,%r6
  60:	34 02 00 00 9a 02 00 00 	div32 %r2,0x29a
  68:	34 03 00 00 66 fd ff ff 	div32 %r3,-666
  70:	34 04 00 00 ef be ad 7e 	div32 %r4,0x7eadbeef
  78:	3c 65 00 00 00 00 00 00 	div32 %r5,%r6
  80:	44 02 00 00 9a 02 00 00 	or32 %r2,0x29a
  88:	44 03 00 00 66 fd ff ff 	or32 %r3,-666
  90:	44 04 00 00 ef be ad 7e 	or32 %r4,0x7eadbeef
  98:	4c 65 00 00 00 00 00 00 	or32 %r5,%r6
  a0:	54 02 00 00 9a 02 00 00 	and32 %r2,0x29a
  a8:	54 03 00 00 66 fd ff ff 	and32 %r3,-666
  b0:	54 04 00 00 ef be ad 7e 	and32 %r4,0x7eadbeef
  b8:	5c 65 00 00 00 00 00 00 	and32 %r5,%r6
  c0:	64 02 00 00 9a 02 00 00 	lsh32 %r2,0x29a
  c8:	64 03 00 00 66 fd ff ff 	lsh32 %r3,-666
  d0:	64 04 00 00 ef be ad 7e 	lsh32 %r4,0x7eadbeef
  d8:	6c 65 00 00 00 00 00 00 	lsh32 %r5,%r6
  e0:	74 02 00 00 9a 02 00 00 	rsh32 %r2,0x29a
  e8:	74 03 00 00 66 fd ff ff 	rsh32 %r3,-666
  f0:	74 04 00 00 ef be ad 7e 	rsh32 %r4,0x7eadbeef
  f8:	7c 65 00 00 00 00 00 00 	rsh32 %r5,%r6
 100:	94 02 00 00 9a 02 00 00 	mod32 %r2,0x29a
 108:	94 03 00 00 66 fd ff ff 	mod32 %r3,-666
 110:	94 04 00 00 ef be ad 7e 	mod32 %r4,0x7eadbeef
 118:	9c 65 00 00 00 00 00 00 	mod32 %r5,%r6
 120:	a4 02 00 00 9a 02 00 00 	xor32 %r2,0x29a
 128:	a4 03 00 00 66 fd ff ff 	xor32 %r3,-666
 130:	a4 04 00 00 ef be ad 7e 	xor32 %r4,0x7eadbeef
 138:	ac 65 00 00 00 00 00 00 	xor32 %r5,%r6
 140:	b4 02 00 00 9a 02 00 00 	mov32 %r2,0x29a
 148:	b4 03 00 00 66 fd ff ff 	mov32 %r3,-666
 150:	b4 04 00 00 ef be ad 7e 	mov32 %r4,0x7eadbeef
 158:	bc 65 00 00 00 00 00 00 	mov32 %r5,%r6
 160:	c4 02 00 00 9a 02 00 00 	arsh32 %r2,0x29a
 168:	c4 03 00 00 66 fd ff ff 	arsh32 %r3,-666
 170:	c4 04 00 00 ef be ad 7e 	arsh32 %r4,0x7eadbeef
 178:	cc 65 00 00 00 00 00 00 	arsh32 %r5,%r6
 180:	84 02 00 00 00 00 00 00 	neg32 %r2
 188:	d4 09 00 00 10 00 00 00 	endle %r9,16
 190:	d4 08 00 00 20 00 00 00 	endle %r8,32
 198:	d4 07 00 00 40 00 00 00 	endle %r7,64
 1a0:	dc 06 00 00 10 00 00 00 	endbe %r6,16
 1a8:	dc 05 00 00 20 00 00 00 	endbe %r5,32
 1b0:	dc 04 00 00 40 00 00 00 	endbe %r4,64
