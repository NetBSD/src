#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	7980009b 	ldrsh	x27, \[x4\]
			0: R_AARCH64_TLSLD_LDST16_DTPREL_LO12	sym

