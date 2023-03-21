#as: --EB
#source: alu32.s
#objdump: -dr
#name: eBPF ALU instructions, big-endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	04 20 00 00 00 00 02 9a 	add32 %r2,0x29a
   8:	04 30 00 00 ff ff fd 66 	add32 %r3,-666
  10:	04 40 00 00 7e ad be ef 	add32 %r4,0x7eadbeef
  18:	0c 56 00 00 00 00 00 00 	add32 %r5,%r6
  20:	14 20 00 00 00 00 02 9a 	sub32 %r2,0x29a
  28:	14 30 00 00 ff ff fd 66 	sub32 %r3,-666
  30:	14 40 00 00 7e ad be ef 	sub32 %r4,0x7eadbeef
  38:	1c 56 00 00 00 00 00 00 	sub32 %r5,%r6
  40:	24 20 00 00 00 00 02 9a 	mul32 %r2,0x29a
  48:	24 30 00 00 ff ff fd 66 	mul32 %r3,-666
  50:	24 40 00 00 7e ad be ef 	mul32 %r4,0x7eadbeef
  58:	2c 56 00 00 00 00 00 00 	mul32 %r5,%r6
  60:	34 20 00 00 00 00 02 9a 	div32 %r2,0x29a
  68:	34 30 00 00 ff ff fd 66 	div32 %r3,-666
  70:	34 40 00 00 7e ad be ef 	div32 %r4,0x7eadbeef
  78:	3c 56 00 00 00 00 00 00 	div32 %r5,%r6
  80:	44 20 00 00 00 00 02 9a 	or32 %r2,0x29a
  88:	44 30 00 00 ff ff fd 66 	or32 %r3,-666
  90:	44 40 00 00 7e ad be ef 	or32 %r4,0x7eadbeef
  98:	4c 56 00 00 00 00 00 00 	or32 %r5,%r6
  a0:	54 20 00 00 00 00 02 9a 	and32 %r2,0x29a
  a8:	54 30 00 00 ff ff fd 66 	and32 %r3,-666
  b0:	54 40 00 00 7e ad be ef 	and32 %r4,0x7eadbeef
  b8:	5c 56 00 00 00 00 00 00 	and32 %r5,%r6
  c0:	64 20 00 00 00 00 02 9a 	lsh32 %r2,0x29a
  c8:	64 30 00 00 ff ff fd 66 	lsh32 %r3,-666
  d0:	64 40 00 00 7e ad be ef 	lsh32 %r4,0x7eadbeef
  d8:	6c 56 00 00 00 00 00 00 	lsh32 %r5,%r6
  e0:	74 20 00 00 00 00 02 9a 	rsh32 %r2,0x29a
  e8:	74 30 00 00 ff ff fd 66 	rsh32 %r3,-666
  f0:	74 40 00 00 7e ad be ef 	rsh32 %r4,0x7eadbeef
  f8:	7c 56 00 00 00 00 00 00 	rsh32 %r5,%r6
 100:	94 20 00 00 00 00 02 9a 	mod32 %r2,0x29a
 108:	94 30 00 00 ff ff fd 66 	mod32 %r3,-666
 110:	94 40 00 00 7e ad be ef 	mod32 %r4,0x7eadbeef
 118:	9c 56 00 00 00 00 00 00 	mod32 %r5,%r6
 120:	a4 20 00 00 00 00 02 9a 	xor32 %r2,0x29a
 128:	a4 30 00 00 ff ff fd 66 	xor32 %r3,-666
 130:	a4 40 00 00 7e ad be ef 	xor32 %r4,0x7eadbeef
 138:	ac 56 00 00 00 00 00 00 	xor32 %r5,%r6
 140:	b4 20 00 00 00 00 02 9a 	mov32 %r2,0x29a
 148:	b4 30 00 00 ff ff fd 66 	mov32 %r3,-666
 150:	b4 40 00 00 7e ad be ef 	mov32 %r4,0x7eadbeef
 158:	bc 56 00 00 00 00 00 00 	mov32 %r5,%r6
 160:	c4 20 00 00 00 00 02 9a 	arsh32 %r2,0x29a
 168:	c4 30 00 00 ff ff fd 66 	arsh32 %r3,-666
 170:	c4 40 00 00 7e ad be ef 	arsh32 %r4,0x7eadbeef
 178:	cc 56 00 00 00 00 00 00 	arsh32 %r5,%r6
 180:	84 20 00 00 00 00 00 00 	neg32 %r2
 188:	d4 90 00 00 00 00 00 10 	endle %r9,16
 190:	d4 80 00 00 00 00 00 20 	endle %r8,32
 198:	d4 70 00 00 00 00 00 40 	endle %r7,64
 1a0:	dc 60 00 00 00 00 00 10 	endbe %r6,16
 1a8:	dc 50 00 00 00 00 00 20 	endbe %r5,32
 1b0:	dc 40 00 00 00 00 00 40 	endbe %r4,64
