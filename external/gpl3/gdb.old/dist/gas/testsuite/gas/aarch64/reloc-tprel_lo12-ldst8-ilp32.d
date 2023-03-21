#as: -mabi=ilp32
#source: reloc-tprel_lo12-ldst8.s
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

00000000 <.*>:
   0:	39800115 	ldrsb	x21, \[x8\]
			0: R_AARCH64_P32_TLSLE_LDST8_TPREL_LO12	sym
