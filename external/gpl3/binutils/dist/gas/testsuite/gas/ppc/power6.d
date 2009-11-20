#as: -a32 -mpower6
#objdump: -dr -Mpower6
#name: POWER6 tests (includes DFP and Altivec)

.*: +file format elf32-powerpc.*

Disassembly of section \.text:

0+00 <start>:
   0:	4c 00 03 24 	doze
   4:	4c 00 03 64 	nap
   8:	4c 00 03 a4 	sleep
   c:	4c 00 03 e4 	rvwinkle
  10:	7c 83 01 34 	prtyw   r3,r4
  14:	7d cd 01 74 	prtyd   r13,r14
  18:	7d 5c 02 a6 	mfcfar  r10
  1c:	7d 7c 03 a6 	mtcfar  r11
  20:	7c 83 2b f8 	cmpb    r3,r4,r5
  24:	7c c0 3c be 	mffgpr  f6,r7
  28:	7d 00 4d be 	mftgpr  r8,f9
  2c:	7d 4b 66 2a 	lwzcix  r10,r11,r12
  30:	7d ae 7e 2e 	lfdpx   f13,r14,r15
  34:	ee 11 90 04 	dadd    f16,f17,f18
  38:	fe 96 c0 04 	daddq   f20,f22,f24
  3c:	7c 60 06 6c 	dss     3
  40:	7e 00 06 6c 	dssall
  44:	7c 25 22 ac 	dst     r5,r4,1
  48:	7e 08 3a ac 	dstt    r8,r7,0
  4c:	7c 65 32 ec 	dstst   r5,r6,3
  50:	7e 44 2a ec 	dststt  r4,r5,2
  54:	00 00 02 00 	attn
  58:	7c 6f f1 20 	mtcr    r3
  5c:	7c 6f f1 20 	mtcr    r3
  60:	7c 68 11 20 	mtcrf   129,r3
  64:	7c 70 11 20 	mtocrf  1,r3
  68:	7c 70 21 20 	mtocrf  2,r3
  6c:	7c 70 41 20 	mtocrf  4,r3
  70:	7c 70 81 20 	mtocrf  8,r3
  74:	7c 71 01 20 	mtocrf  16,r3
  78:	7c 72 01 20 	mtocrf  32,r3
  7c:	7c 74 01 20 	mtocrf  64,r3
  80:	7c 78 01 20 	mtocrf  128,r3
  84:	7c 60 00 26 	mfcr    r3
  88:	7c 70 10 26 	mfocrf  r3,1
  8c:	7c 70 20 26 	mfocrf  r3,2
  90:	7c 70 40 26 	mfocrf  r3,4
  94:	7c 70 80 26 	mfocrf  r3,8
  98:	7c 71 00 26 	mfocrf  r3,16
  9c:	7c 72 00 26 	mfocrf  r3,32
  a0:	7c 74 00 26 	mfocrf  r3,64
  a4:	7c 78 00 26 	mfocrf  r3,128
  a8:	7c 01 17 ec 	dcbz    r1,r2
  ac:	7c 23 27 ec 	dcbzl   r3,r4
  b0:	7c 05 37 ec 	dcbz    r5,r6
  b4:	fc 0c 55 8e 	mtfsf   6,f10
  b8:	fc 0c 5d 8f 	mtfsf.  6,f11
  bc:	fc 0c 55 8e 	mtfsf   6,f10
  c0:	fc 0c 5d 8f 	mtfsf.  6,f11
  c4:	fc 0d 55 8e 	mtfsf   6,f10,0,1
  c8:	fc 0d 5d 8f 	mtfsf.  6,f11,0,1
  cc:	fe 0c 55 8e 	mtfsf   6,f10,1,0
  d0:	fe 0c 5d 8f 	mtfsf.  6,f11,1,0
  d4:	ff 00 01 0c 	mtfsfi  6,0
  d8:	ff 00 f1 0d 	mtfsfi. 6,15
  dc:	ff 00 01 0c 	mtfsfi  6,0
  e0:	ff 00 f1 0d 	mtfsfi. 6,15
  e4:	ff 01 01 0c 	mtfsfi  6,0,1
  e8:	ff 01 f1 0d 	mtfsfi. 6,15,1
  ec:	7d 6a 02 74 	cbcdtd  r10,r11
  f0:	7d 6a 02 34 	cdtbcd  r10,r11
  f4:	7d 4b 60 94 	addg6s  r10,r11,r12
