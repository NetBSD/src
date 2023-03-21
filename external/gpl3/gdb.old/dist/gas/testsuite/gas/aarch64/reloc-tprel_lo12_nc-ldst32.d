#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	b98000f4 	ldrsw	x20, \[x7\]
			0: R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC	sym
