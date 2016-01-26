#objdump: -dr

dump\.o:     file format .*


Disassembly of section \.text:

00000000 <\.text>:
   0:	fd 05 00                      	maclo	r0, r0
   3:	fd 05 0f                      	maclo	r0, r15
   6:	fd 05 f0                      	maclo	r15, r0
   9:	fd 05 ff                      	maclo	r15, r15
