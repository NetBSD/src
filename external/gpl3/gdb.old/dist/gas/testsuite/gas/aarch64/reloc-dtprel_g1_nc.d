#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	f2a00009 	movk	x9, #0x0, lsl #16
			0: R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC	x
