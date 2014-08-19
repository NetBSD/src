#as: -Av9v
#objdump: -dr
#name: sparc CBCOND

.*: +file format .*sparc.*

Disassembly of section .text:

0+ <.text>:
   0:	12 c2 47 0a 	cwbe  %o1, %o2, 0xe0
   4:	12 c2 66 e2 	cwbe  %o1, 2, 0xe0
   8:	12 e2 86 cb 	cxbe  %o2, %o3, 0xe0
   c:	12 e2 a6 a3 	cxbe  %o2, 3, 0xe0
  10:	14 c2 c6 8c 	cwble  %o3, %o4, 0xe0
  14:	14 c2 e6 64 	cwble  %o3, 4, 0xe0
  18:	14 e3 06 4d 	cxble  %o4, %o5, 0xe0
  1c:	14 e3 26 25 	cxble  %o4, 5, 0xe0
  20:	16 c3 46 10 	cwbl  %o5, %l0, 0xe0
  24:	16 c3 65 e6 	cwbl  %o5, 6, 0xe0
  28:	16 e4 05 d1 	cxbl  %l0, %l1, 0xe0
  2c:	16 e4 25 a7 	cxbl  %l0, 7, 0xe0
  30:	18 c4 45 92 	cwbleu  %l1, %l2, 0xe0
  34:	18 c4 65 68 	cwbleu  %l1, 8, 0xe0
  38:	18 e4 85 53 	cxbleu  %l2, %l3, 0xe0
  3c:	18 e4 a5 29 	cxbleu  %l2, 9, 0xe0
  40:	1a c4 c5 14 	cwbcs  %l3, %l4, 0xe0
  44:	1a c4 e4 ea 	cwbcs  %l3, 0xa, 0xe0
  48:	1a e5 04 d5 	cxbcs  %l4, %l5, 0xe0
  4c:	1a e5 24 ab 	cxbcs  %l4, 0xb, 0xe0
  50:	1c c5 44 96 	cwbneg  %l5, %l6, 0xe0
  54:	1c c5 64 6c 	cwbneg  %l5, 0xc, 0xe0
  58:	1c e5 84 57 	cxbneg  %l6, %l7, 0xe0
  5c:	1c e5 a4 2d 	cxbneg  %l6, 0xd, 0xe0
  60:	1e c5 c4 18 	cwbvs  %l7, %i0, 0xe0
  64:	1e c5 e3 ee 	cwbvs  %l7, 0xe, 0xe0
  68:	1e e6 03 d9 	cxbvs  %i0, %i1, 0xe0
  6c:	1e e6 23 af 	cxbvs  %i0, 0xf, 0xe0
  70:	32 c6 43 9a 	cwbne  %i1, %i2, 0xe0
  74:	32 c6 63 70 	cwbne  %i1, 0x10, 0xe0
  78:	32 e6 83 5b 	cxbne  %i2, %i3, 0xe0
  7c:	32 e6 a3 31 	cxbne  %i2, 0x11, 0xe0
  80:	34 c6 c3 1c 	cwbg  %i3, %i4, 0xe0
  84:	34 c6 e2 f2 	cwbg  %i3, 0x12, 0xe0
  88:	34 e7 02 dd 	cxbg  %i4, %i5, 0xe0
  8c:	34 e7 22 b3 	cxbg  %i4, 0x13, 0xe0
  90:	36 c7 42 88 	cwbge  %i5, %o0, 0xe0
  94:	36 c7 62 74 	cwbge  %i5, 0x14, 0xe0
  98:	36 e2 02 49 	cxbge  %o0, %o1, 0xe0
  9c:	36 e2 22 35 	cxbge  %o0, 0x15, 0xe0
  a0:	38 c2 42 0a 	cwbgu  %o1, %o2, 0xe0
  a4:	38 c2 61 f6 	cwbgu  %o1, 0x16, 0xe0
  a8:	38 e2 81 cb 	cxbgu  %o2, %o3, 0xe0
  ac:	38 e2 a1 b6 	cxbgu  %o2, 0x16, 0xe0
  b0:	3a c2 c1 8c 	cwbcc  %o3, %o4, 0xe0
  b4:	3a c2 e1 77 	cwbcc  %o3, 0x17, 0xe0
  b8:	3a e3 01 4d 	cxbcc  %o4, %o5, 0xe0
  bc:	3a e3 21 38 	cxbcc  %o4, 0x18, 0xe0
  c0:	3c c3 41 10 	cwbpos  %o5, %l0, 0xe0
  c4:	3c c3 60 f9 	cwbpos  %o5, 0x19, 0xe0
  c8:	3c e4 00 d1 	cxbpos  %l0, %l1, 0xe0
  cc:	3c e4 20 b9 	cxbpos  %l0, 0x19, 0xe0
  d0:	3e c4 40 92 	cwbvc  %l1, %l2, 0xe0
  d4:	3e c4 60 7a 	cwbvc  %l1, 0x1a, 0xe0
  d8:	3e e4 80 53 	cxbvc  %l2, %l3, 0xe0
  dc:	3e e4 a0 3b 	cxbvc  %l2, 0x1b, 0xe0
  e0:	01 00 00 00 	nop 
