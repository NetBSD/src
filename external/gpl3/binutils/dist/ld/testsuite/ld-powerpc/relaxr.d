
.*:     file format .*

Disassembly of section .text:

00000000 <_start>:
   0:	48 00 00 15 	bl      14 <_start\+0x14>
   4:	48 00 00 21 	bl      24 <_start\+0x24>
   8:	48 00 00 0d 	bl      14 <_start\+0x14>
			8: R_PPC_NONE	\*ABS\*
   c:	48 00 00 19 	bl      24 <_start\+0x24>
			c: R_PPC_NONE	\*ABS\*
  10:	48 00 00 00 	b       10 <_start\+0x10>
			10: R_PPC_REL24	_start
  14:	3d 80 00 00 	lis     r12,0
			16: R_PPC_ADDR16_HA	near
  18:	39 8c 00 00 	addi    r12,r12,0
			1a: R_PPC_ADDR16_LO	near
  1c:	7d 89 03 a6 	mtctr   r12
  20:	4e 80 04 20 	bctr
  24:	3d 80 00 00 	lis     r12,0
			26: R_PPC_ADDR16_HA	far
  28:	39 8c 00 00 	addi    r12,r12,0
			2a: R_PPC_ADDR16_LO	far
  2c:	7d 89 03 a6 	mtctr   r12
  30:	4e 80 04 20 	bctr
