#as: --EB
#source: alu.s
#objdump: -dr
#name: eBPF ALU64 instructions, big endian

.*: +file format .*bpf.*

Disassembly of section .text:

0+ <.text>:
   0:	07 20 00 00 00 00 02 9a 	add %r2,0x29a
   8:	07 30 00 00 ff ff fd 66 	add %r3,-666
  10:	07 40 00 00 7e ad be ef 	add %r4,0x7eadbeef
  18:	0f 56 00 00 00 00 00 00 	add %r5,%r6
  20:	17 20 00 00 00 00 02 9a 	sub %r2,0x29a
  28:	17 30 00 00 ff ff fd 66 	sub %r3,-666
  30:	17 40 00 00 7e ad be ef 	sub %r4,0x7eadbeef
  38:	1f 56 00 00 00 00 00 00 	sub %r5,%r6
  40:	27 20 00 00 00 00 02 9a 	mul %r2,0x29a
  48:	27 30 00 00 ff ff fd 66 	mul %r3,-666
  50:	27 40 00 00 7e ad be ef 	mul %r4,0x7eadbeef
  58:	2f 56 00 00 00 00 00 00 	mul %r5,%r6
  60:	37 20 00 00 00 00 02 9a 	div %r2,0x29a
  68:	37 30 00 00 ff ff fd 66 	div %r3,-666
  70:	37 40 00 00 7e ad be ef 	div %r4,0x7eadbeef
  78:	3f 56 00 00 00 00 00 00 	div %r5,%r6
  80:	47 20 00 00 00 00 02 9a 	or %r2,0x29a
  88:	47 30 00 00 ff ff fd 66 	or %r3,-666
  90:	47 40 00 00 7e ad be ef 	or %r4,0x7eadbeef
  98:	4f 56 00 00 00 00 00 00 	or %r5,%r6
  a0:	57 20 00 00 00 00 02 9a 	and %r2,0x29a
  a8:	57 30 00 00 ff ff fd 66 	and %r3,-666
  b0:	57 40 00 00 7e ad be ef 	and %r4,0x7eadbeef
  b8:	5f 56 00 00 00 00 00 00 	and %r5,%r6
  c0:	67 20 00 00 00 00 02 9a 	lsh %r2,0x29a
  c8:	67 30 00 00 ff ff fd 66 	lsh %r3,-666
  d0:	67 40 00 00 7e ad be ef 	lsh %r4,0x7eadbeef
  d8:	6f 56 00 00 00 00 00 00 	lsh %r5,%r6
  e0:	77 20 00 00 00 00 02 9a 	rsh %r2,0x29a
  e8:	77 30 00 00 ff ff fd 66 	rsh %r3,-666
  f0:	77 40 00 00 7e ad be ef 	rsh %r4,0x7eadbeef
  f8:	7f 56 00 00 00 00 00 00 	rsh %r5,%r6
 100:	97 20 00 00 00 00 02 9a 	mod %r2,0x29a
 108:	97 30 00 00 ff ff fd 66 	mod %r3,-666
 110:	97 40 00 00 7e ad be ef 	mod %r4,0x7eadbeef
 118:	9f 56 00 00 00 00 00 00 	mod %r5,%r6
 120:	a7 20 00 00 00 00 02 9a 	xor %r2,0x29a
 128:	a7 30 00 00 ff ff fd 66 	xor %r3,-666
 130:	a7 40 00 00 7e ad be ef 	xor %r4,0x7eadbeef
 138:	af 56 00 00 00 00 00 00 	xor %r5,%r6
 140:	b7 20 00 00 00 00 02 9a 	mov %r2,0x29a
 148:	b7 30 00 00 ff ff fd 66 	mov %r3,-666
 150:	b7 40 00 00 7e ad be ef 	mov %r4,0x7eadbeef
 158:	bf 56 00 00 00 00 00 00 	mov %r5,%r6
 160:	c7 20 00 00 00 00 02 9a 	arsh %r2,0x29a
 168:	c7 30 00 00 ff ff fd 66 	arsh %r3,-666
 170:	c7 40 00 00 7e ad be ef 	arsh %r4,0x7eadbeef
 178:	cf 56 00 00 00 00 00 00 	arsh %r5,%r6
 180:	87 20 00 00 00 00 00 00 	neg %r2
