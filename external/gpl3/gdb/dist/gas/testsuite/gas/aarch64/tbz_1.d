#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
   0:	36080000 	tbz	w0, #1, 0 <bar>
			0: R_AARCH64_(P32_|)TSTBR14	bar\+0x8000
