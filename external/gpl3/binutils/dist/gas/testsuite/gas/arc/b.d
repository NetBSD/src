#as: -mcpu=arc700
#objdump: -dr --show-raw-insn

.*: +file format .*arc.*

Disassembly of section .text:

00000000 <text_label>:
   0:	0001 0000           	b	0 <text_label>
   4:	07fc ffc0           	b	-4
   8:	07f8 ffc0           	b	-8
   c:	07f4 ffc1           	beq	-12
  10:	07f0 ffc1           	beq	-16
  14:	07ec ffc2           	bne	-20
  18:	07e8 ffc2           	bne	-24
  1c:	07e4 ffc3           	bp	-28
  20:	07e0 ffc3           	bp	-32
  24:	07dc ffc4           	bn	-36
  28:	07d8 ffc4           	bn	-40
  2c:	07d4 ffc5           	bc	-44
  30:	07d0 ffc5           	bc	-48
  34:	07cc ffc5           	bc	-52
  38:	07c8 ffc6           	bnc	-56
  3c:	07c4 ffc6           	bnc	-60
  40:	07c0 ffc6           	bnc	-64
  44:	07bc ffc7           	bv	-68
  48:	07b8 ffc7           	bv	-72
  4c:	07b4 ffc8           	bnv	-76
  50:	07b0 ffc8           	bnv	-80
  54:	07ac ffc9           	bgt	-84
  58:	07a8 ffca           	bge	-88
  5c:	07a4 ffcb           	blt	-92
  60:	07a0 ffcc           	ble	-96
  64:	079c ffcd           	bhi	-100
  68:	0798 ffce           	bls	-104
  6c:	0794 ffcf           	bpnz	-108
  70:	0791 ffef           	b.d	0 <text_label>
  74:	264a 7000           	mov	0,0
  78:	0789 ffcf           	b	0 <text_label>
  7c:	0785 ffef           	b.d	0 <text_label>
  80:	264a 7000           	mov	0,0
  84:	077c ffe1           	b.deq	-132
  88:	264a 7000           	mov	0,0
  8c:	0774 ffc2           	bne	-140
  90:	0770 ffe6           	b.dnc	-144
  94:	264a 7000           	mov	0,0
