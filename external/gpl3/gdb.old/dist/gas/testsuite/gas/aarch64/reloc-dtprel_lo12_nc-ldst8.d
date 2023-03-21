#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	3940005d 	ldrb	w29, \[x2\]
			0: R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC	sym

