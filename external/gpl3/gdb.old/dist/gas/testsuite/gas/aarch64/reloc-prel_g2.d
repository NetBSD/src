#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	8a000000 	and	x0, x0, x0
   4:	92400000 	and	x0, x0, #0x1
   8:	d2c00004 	movz	x4, #0x0, lsl #32
			8: R_AARCH64_MOVW_PREL_G2	tempy
   c:	d2c00007 	movz	x7, #0x0, lsl #32
			c: R_AARCH64_MOVW_PREL_G2	tempy2
  10:	d2c00011 	movz	x17, #0x0, lsl #32
			10: R_AARCH64_MOVW_PREL_G2	tempy3
