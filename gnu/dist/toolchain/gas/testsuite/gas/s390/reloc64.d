#name: s390x reloc
#objdump: -dr

.*: +file format .*

Disassembly of section .text:

.* <foo>:
   0:	c0 e5 00 00 00 00 [ 	]*brasl	%r14,0 <foo>
[ 	]*2: R_390_PC32DBL	test_R_390_PC32DBL\+0x2
   6:	c0 e5 00 00 00 00 [ 	]*brasl	%r14,6 <foo\+0x6>
[ 	]*8: R_390_PC32DBL	test_R_390_PLT32DBL\+0x2
[ 	]*...
[ 	]*c: R_390_64	test_R_390_64
[ 	]*14: R_390_PC64	test_R_390_PC64\+0x14
[ 	]*1c: R_390_GOT64	test_R_390_GOT64
[ 	]*24: R_390_PLT64	test_R_390_PLT64
  2c:	c0 10 00 00 00 00 [ 	]*larl	%r1,2c <foo\+0x2c>
[ 	]*2e: R_390_GOTENT	test_R_390_GOTENT\+0x2
  32:	07 07 [ 	]*bcr	0,%r7
