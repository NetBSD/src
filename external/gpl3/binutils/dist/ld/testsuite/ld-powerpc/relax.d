
.*:     file format .*

Disassembly of section .text:

00000000 <_start>:
   0:	48 00 43 21 	bl      4320 <near>
   4:	48 00 00 11 	bl      14 <_start\+0x14>
   8:	48 00 43 19 	bl      4320 <near>
   c:	48 00 00 09 	bl      14 <_start\+0x14>
  10:	4b ff ff f0 	b       0 <.*>
  14:	3d 80 80 00 	lis     r12,-32768
  18:	39 8c 12 34 	addi    r12,r12,4660
  1c:	7d 89 03 a6 	mtctr   r12
  20:	4e 80 04 20 	bctr
