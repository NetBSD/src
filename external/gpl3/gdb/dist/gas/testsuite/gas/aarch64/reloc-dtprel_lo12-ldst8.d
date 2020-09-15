#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	39800115 	ldrsb	x21, \[x8\]
			0: R_AARCH64_TLSLD_LDST8_DTPREL_LO12	sym

