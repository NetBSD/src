#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	d2c0000a 	movz	x10, #0x0, lsl #32
			0: R_AARCH64_TLSLD_MOVW_DTPREL_G2	x
