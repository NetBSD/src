#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	d2a0001c 	movz	x28, #0x0, lsl #16
			0: R_AARCH64_TLSGD_MOVW_G1	var
