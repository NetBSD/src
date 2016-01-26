#objdump: -dr

dump\.o:     file format .*


Disassembly of section \.text:

00000000 <\.text>:
   0:	fd 01 00                      	mullo	r0, r0
   3:	fd 01 0f                      	mullo	r0, r15
   6:	fd 01 f0                      	mullo	r15, r0
   9:	fd 01 ff                      	mullo	r15, r15
