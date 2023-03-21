#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	7980009b 	ldrsh	x27, \[x4\]
			0: R_AARCH64_TLSLE_LDST16_TPREL_LO12	sym
