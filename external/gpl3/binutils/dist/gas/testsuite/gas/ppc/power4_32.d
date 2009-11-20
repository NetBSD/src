#objdump: -d -Mpower4
#as: -a32 -mpower4
#name: Power4 instructions

.*: +file format elf32-powerpc.*

Disassembly of section \.text:

0+00 <start>:
   0:	80 c7 00 00 	lwz     r6,0\(r7\)
   4:	80 c7 00 10 	lwz     r6,16\(r7\)
   8:	80 c7 ff f0 	lwz     r6,-16\(r7\)
   c:	80 c7 80 00 	lwz     r6,-32768\(r7\)
  10:	80 c7 7f f0 	lwz     r6,32752\(r7\)
  14:	90 c7 00 00 	stw     r6,0\(r7\)
  18:	90 c7 00 10 	stw     r6,16\(r7\)
  1c:	90 c7 ff f0 	stw     r6,-16\(r7\)
  20:	90 c7 80 00 	stw     r6,-32768\(r7\)
  24:	90 c7 7f f0 	stw     r6,32752\(r7\)
  28:	00 00 02 00 	attn
  2c:	7c 6f f1 20 	mtcr    r3
  30:	7c 6f f1 20 	mtcr    r3
  34:	7c 68 11 20 	mtcrf   129,r3
  38:	7c 70 11 20 	mtocrf  1,r3
  3c:	7c 70 21 20 	mtocrf  2,r3
  40:	7c 70 41 20 	mtocrf  4,r3
  44:	7c 70 81 20 	mtocrf  8,r3
  48:	7c 71 01 20 	mtocrf  16,r3
  4c:	7c 72 01 20 	mtocrf  32,r3
  50:	7c 74 01 20 	mtocrf  64,r3
  54:	7c 78 01 20 	mtocrf  128,r3
  58:	7c 60 00 26 	mfcr    r3
  5c:	7c 70 10 26 	mfocrf  r3,1
  60:	7c 70 20 26 	mfocrf  r3,2
  64:	7c 70 40 26 	mfocrf  r3,4
  68:	7c 70 80 26 	mfocrf  r3,8
  6c:	7c 71 00 26 	mfocrf  r3,16
  70:	7c 72 00 26 	mfocrf  r3,32
  74:	7c 74 00 26 	mfocrf  r3,64
  78:	7c 78 00 26 	mfocrf  r3,128
  7c:	7c 01 17 ec 	dcbz    r1,r2
  80:	7c 23 27 ec 	dcbzl   r3,r4
  84:	7c 05 37 ec 	dcbz    r5,r6
  88:	7c 05 32 2c 	dcbt    r5,r6
  8c:	7c 05 32 2c 	dcbt    r5,r6
  90:	7d 05 32 2c 	dcbt    r5,r6,8
