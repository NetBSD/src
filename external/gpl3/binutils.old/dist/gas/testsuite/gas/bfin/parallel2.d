#objdump: -d
#name: parallel2
.*: +file format .*

Disassembly of section .text:

00000000 <.text>:
   0:	08 cc 3f c0 	A0 = A1 \|\| P0 = \[SP \+ 0x14\] \|\| NOP;
   4:	70 ad 00 00 
   8:	08 cc 3f e0 	A1 = A0 \|\| P0 = \[P5 \+ 0x18\] \|\| NOP;
   c:	a8 ad 00 00 
  10:	09 cc 00 20 	A0 = R0 \|\| P0 = \[P4 \+ 0x1c\] \|\| NOP;
  14:	e0 ad 00 00 
  18:	09 cc 08 a0 	A1 = R1 \|\| P0 = \[P3 \+ 0x20\] \|\| NOP;
  1c:	18 ae 00 00 
  20:	8b c8 00 39 	R4 = A0 \(FU\) \|\| P0 = \[P3 \+ 0x24\] \|\| NOP;
  24:	58 ae 00 00 
  28:	2f c9 00 19 	R5 = A1 \(ISS2\) \|\| P0 = \[P3 \+ 0x28\] \|\| NOP;
  2c:	98 ae 00 00 
  30:	0b c8 80 39 	R6 = A0 \|\| P0 = \[P4 \+ 0x2c\] \|\| NOP;
  34:	e0 ae 00 00 
  38:	0f c8 80 19 	R7 = A1 \|\| P0 = \[P4 \+ 0x30\] \|\| NOP;
  3c:	20 af 00 00 
  40:	0f c8 80 39 	R7 = A1, R6 = A0 \|\| P0 = \[P4 \+ 0x34\] \|\| NOP;
  44:	60 af 00 00 
  48:	8f c8 00 38 	R1 = A1, R0 = A0 \(FU\) \|\| P0 = \[P4 \+ 0x38\] \|\| NOP;
  4c:	a0 af 00 00 
  50:	09 cc 28 40 	A0.X = R5.L \|\| P0 = \[P4 \+ 0x3c\] \|\| NOP;
  54:	e0 af 00 00 
  58:	09 cc 10 c0 	A1.X = R2.L \|\| R0 = \[I0 \+\+ M0\] \|\| NOP;
  5c:	80 9d 00 00 
  60:	0a cc 3f 00 	R0.L = A0.X \|\| R1 = \[I0 \+\+ M1\] \|\| NOP;
  64:	a1 9d 00 00 
  68:	0a cc 3f 4e 	R7.L = A1.X \|\| R0 = \[I0 \+\+ M2\] \|\| NOP;
  6c:	c0 9d 00 00 
  70:	09 cc 18 00 	A0.L = R3.L \|\| R0 = \[I0 \+\+ M3\] \|\| NOP;
  74:	e0 9d 00 00 
  78:	09 cc 20 80 	A1.L = R4.L \|\| R0 = \[I1 \+\+ M3\] \|\| NOP;
  7c:	e8 9d 00 00 
  80:	29 cc 30 00 	A0.H = R6.H \|\| R0 = \[I1 \+\+ M2\] \|\| NOP;
  84:	c8 9d 00 00 
  88:	29 cc 28 80 	A1.H = R5.H \|\| R0 = \[I1 \+\+ M1\] \|\| NOP;
  8c:	a8 9d 00 00 
  90:	83 c9 00 38 	R0.L = A0 \(IU\) \|\| R4 = \[I1 \+\+ M0\] \|\| NOP;
  94:	8c 9d 00 00 
  98:	27 c8 40 18 	R1.H = A1 \(S2RND\) \|\| R0 = \[I2 \+\+ M0\] \|\| NOP;
  9c:	90 9d 00 00 
  a0:	07 c8 40 18 	R1.H = A1 \|\| R0 = \[I2 \+\+ M1\] \|\| NOP;
  a4:	b0 9d 00 00 
  a8:	67 c9 80 38 	R2.H = A1, R2.L = A0 \(IH\) \|\| R0 = \[I2 \+\+ M2\] \|\| NOP;
  ac:	d0 9d 00 00 
  b0:	07 c8 80 38 	R2.H = A1, R2.L = A0 \|\| R0 = \[I2 \+\+ M3\] \|\| NOP;
  b4:	f0 9d 00 00 
  b8:	47 c8 00 38 	R0.H = A1, R0.L = A0 \(T\) \|\| R5 = \[I3 \+\+ M0\] \|\| NOP;
  bc:	9d 9d 00 00 
  c0:	87 c8 00 38 	R0.H = A1, R0.L = A0 \(FU\) \|\| R5 = \[I3 \+\+ M1\] \|\| NOP;
  c4:	bd 9d 00 00 
  c8:	07 c9 00 38 	R0.H = A1, R0.L = A0 \(IS\) \|\| R5 = \[I3 \+\+ M2\] \|\| NOP;
  cc:	dd 9d 00 00 
  d0:	07 c8 00 38 	R0.H = A1, R0.L = A0 \|\| R5 = \[I3 \+\+ M3\] \|\| NOP;
  d4:	fd 9d 00 00 
  d8:	83 ce 08 41 	A0 = A0 >> 0x1f \|\| R0 = \[FP -0x20\] \|\| NOP;
  dc:	80 b9 00 00 
  e0:	83 ce f8 00 	A0 = A0 << 0x1f \|\| R0 = \[FP -0x1c\] \|\| NOP;
  e4:	90 b9 00 00 
  e8:	83 ce 00 50 	A1 = A1 >> 0x0 \|\| R0 = \[FP -0x18\] \|\| NOP;
  ec:	a0 b9 00 00 
  f0:	83 ce 00 10 	A1 = A1 << 0x0 \|\| R0 = \[FP -0x14\] \|\| NOP;
  f4:	b0 b9 00 00 
  f8:	82 ce fd 4e 	R7 = R5 << 0x1f \(S\) \|\| R0 = \[FP -0x10\] \|\| NOP;
  fc:	c0 b9 00 00 
 100:	82 ce 52 07 	R3 = R2 >>> 0x16 \|\| R0 = \[FP -0xc\] \|\| NOP;
 104:	d0 b9 00 00 
 108:	80 ce 7a 52 	R1.L = R2.H << 0xf \(S\) \|\| R0 = \[FP -0x8\] \|\| NOP;
 10c:	e0 b9 00 00 
 110:	80 ce f2 2b 	R5.H = R2.L >>> 0x2 \|\| R0 = \[FP -0x4\] \|\| NOP;
 114:	f0 b9 00 00 
 118:	00 ce 14 16 	R3.L = ASHIFT R4.H BY R2.L \|\| R0 = \[FP -0x64\] \|\| NOP;
 11c:	70 b8 00 00 
 120:	00 ce 07 6e 	R7.H = ASHIFT R7.L BY R0.L \(S\) \|\| R0 = \[FP -0x68\] \|\| NOP;
 124:	60 b8 00 00 
 128:	00 ce 07 6e 	R7.H = ASHIFT R7.L BY R0.L \(S\) \|\| R0 = \[FP -0x6c\] \|\| NOP;
 12c:	50 b8 00 00 
 130:	02 ce 15 0c 	R6 = ASHIFT R5 BY R2.L \|\| R0 = \[FP -0x70\] \|\| NOP;
 134:	40 b8 00 00 
 138:	02 ce 0c 40 	R0 = ASHIFT R4 BY R1.L \(S\) \|\| R3 = \[FP -0x74\] \|\| NOP;
 13c:	33 b8 00 00 
 140:	02 ce 1e 44 	R2 = ASHIFT R6 BY R3.L \(S\) \|\| R0 = \[FP -0x78\] \|\| NOP;
 144:	20 b8 00 00 
 148:	03 ce 08 00 	A0 = ASHIFT A0 BY R1.L \|\| R0 = \[FP -0x7c\] \|\| NOP;
 14c:	10 b8 00 00 
 150:	03 ce 00 10 	A1 = ASHIFT A1 BY R0.L \|\| R0 = \[FP -0x80\] \|\| NOP;
 154:	00 b8 00 00 
 158:	80 ce 8a a3 	R1.H = R2.L >> 0xf \|\| R5 = W\[P1--\] \(Z\) \|\| NOP;
 15c:	8d 94 00 00 
 160:	80 ce 00 8e 	R7.L = R0.L << 0x0 \|\| R5 = W\[P2\] \(Z\) \|\| NOP;
 164:	15 95 00 00 
 168:	82 ce 0d 8b 	R5 = R5 >> 0x1f \|\| R7 = W\[P2\+\+\] \(Z\) \|\| NOP;
 16c:	17 94 00 00 
 170:	82 ce 60 80 	R0 = R0 << 0xc \|\| R5 = W\[P2--\] \(Z\) \|\| NOP;
 174:	95 94 00 00 
 178:	83 ce f8 41 	A0 = A0 >> 0x1 \|\| R5 = W\[P2 \+ 0x0\] \(Z\) \|\| NOP;
 17c:	15 a4 00 00 
 180:	83 ce 00 00 	A0 = A0 << 0x0 \|\| R5 = W\[P2 \+ 0x2\] \(Z\) \|\| NOP;
 184:	55 a4 00 00 
 188:	83 ce f8 10 	A1 = A1 << 0x1f \|\| R5 = W\[P2 \+ 0x4\] \(Z\) \|\| NOP;
 18c:	95 a4 00 00 
 190:	83 ce 80 51 	A1 = A1 >> 0x10 \|\| R5 = W\[P2 \+ 0x1e\] \(Z\) \|\| NOP;
 194:	d5 a7 00 00 
 198:	00 ce 02 b2 	R1.H = LSHIFT R2.H BY R0.L \|\| R5 = W\[P2 \+ 0x18\] \(Z\) \|\| NOP;
 19c:	15 a7 00 00 
 1a0:	00 ce 08 90 	R0.L = LSHIFT R0.H BY R1.L \|\| R5 = W\[P2 \+ 0x16\] \(Z\) \|\| NOP;
 1a4:	d5 a6 00 00 
 1a8:	00 ce 16 8e 	R7.L = LSHIFT R6.L BY R2.L \|\| R5 = W\[P2 \+ 0x14\] \(Z\) \|\| NOP;
 1ac:	95 a6 00 00 
 1b0:	02 ce 1c 8a 	R5 = LSHIFT R4 BY R3.L \|\| R4 = W\[P2 \+ 0x12\] \(Z\) \|\| NOP;
 1b4:	54 a6 00 00 
 1b8:	03 ce 30 40 	A0 = LSHIFT A0 BY R6.L \|\| R5 = W\[P2 \+ 0x10\] \(Z\) \|\| NOP;
 1bc:	15 a6 00 00 
 1c0:	03 ce 28 50 	A1 = LSHIFT A1 BY R5.L \|\| R5 = W\[P2 \+ 0xe\] \(Z\) \|\| NOP;
 1c4:	d5 a5 00 00 
 1c8:	82 ce 07 cf 	R7 = ROT R7 BY -0x20 \|\| R5 = W\[P2 \+ 0xc\] \(Z\) \|\| NOP;
 1cc:	95 a5 00 00 
 1d0:	82 ce 0f cd 	R6 = ROT R7 BY -0x1f \|\| R5 = W\[P2 \+ 0xa\] \(Z\) \|\| NOP;
 1d4:	55 a5 00 00 
 1d8:	82 ce ff ca 	R5 = ROT R7 BY 0x1f \|\| R6 = W\[P2 \+ 0x8\] \(Z\) \|\| NOP;
 1dc:	16 a5 00 00 
 1e0:	82 ce f7 c8 	R4 = ROT R7 BY 0x1e \|\| R5 = W\[P2 \+ 0x6\] \(Z\) \|\| NOP;
 1e4:	d5 a4 00 00 
 1e8:	83 ce 00 80 	A0 = ROT A0 BY 0x0 \|\| R5 = W\[P3\] \(Z\) \|\| NOP;
 1ec:	1d 95 00 00 
 1f0:	83 ce 50 80 	A0 = ROT A0 BY 0xa \|\| R5 = W\[P3\+\+\] \(Z\) \|\| NOP;
 1f4:	1d 94 00 00 
 1f8:	83 ce 60 91 	A1 = ROT A1 BY -0x14 \|\| R5 = W\[P3--\] \(Z\) \|\| NOP;
 1fc:	9d 94 00 00 
 200:	83 ce 00 91 	A1 = ROT A1 BY -0x20 \|\| R5 = W\[P4\] \(Z\) \|\| NOP;
 204:	25 95 00 00 
 208:	02 ce 11 c0 	R0 = ROT R1 BY R2.L \|\| R5 = W\[P4\+\+\] \(Z\) \|\| NOP;
 20c:	25 94 00 00 
 210:	02 ce 1c c0 	R0 = ROT R4 BY R3.L \|\| R5 = W\[P4--\] \(Z\) \|\| NOP;
 214:	a5 94 00 00 
 218:	03 ce 38 80 	A0 = ROT A0 BY R7.L \|\| R5 = W\[P5\] \(Z\) \|\| NOP;
 21c:	2d 95 00 00 
 220:	03 ce 30 90 	A1 = ROT A1 BY R6.L \|\| R5 = W\[P5\+\+\] \(Z\) \|\| NOP;
 224:	2d 94 00 00 
 228:	03 c8 00 18 	MNOP \|\| R5 = W\[P5--\] \(Z\) \|\| NOP;
 22c:	ad 94 00 00 
