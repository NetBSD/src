#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0+ <.*>:
   0:	54000000 	b\.eq	0 <bar>  // b\.none
			0: R_AARCH64_(P32_|)CONDBR19	bar\+0x100000
