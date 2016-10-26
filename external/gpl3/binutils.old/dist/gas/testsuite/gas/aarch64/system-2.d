#as: -march=armv8.2-a+profile
#objdump: -dr

.*:     file format .*

Disassembly of section \.text:

0000000000000000 <.*>:
   0:	d503221f 	esb
   4:	d503221f 	esb
   8:	d503223f 	psb	csync
   c:	d503223f 	psb	csync
