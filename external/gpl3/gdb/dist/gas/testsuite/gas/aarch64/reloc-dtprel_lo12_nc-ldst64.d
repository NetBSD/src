#as: -mabi=lp64
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	f940009b 	ldr	x27, \[x4\]
			0: R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC	sym

