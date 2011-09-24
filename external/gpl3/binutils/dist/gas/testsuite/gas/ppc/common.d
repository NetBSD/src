#objdump: -d -Mcom
#as: -a32 -mcom
#name: PowerPC COMMON instructions

.*: +file format elf32-powerpc.*

Disassembly of section \.text:

0+00 <start>:

   0:	7c 83 28 39 	and.    r3,r4,r5
   4:	7c 83 28 38 	and     r3,r4,r5
   8:	7d cd 78 78 	andc    r13,r14,r15
   c:	7e 30 90 79 	andc.   r16,r17,r18
  10:	48 00 00 02 	ba      0 <start>
  14:	40 01 00 00 	bdnzf-  1,14 <start\+0x14>
  18:	40 85 00 02 	blea-   1,0 <start>
  1c:	40 43 00 01 	bdzfl-  3,1c <start\+0x1c>
  20:	41 47 00 03 	bdztla- 7,0 <start>
  24:	4e 80 04 20 	bctr
  28:	4e 80 04 21 	bctrl
  2c:	42 40 00 02 	bdza-   0 <start>
  30:	42 40 00 00 	bdz-    30 <start\+0x30>
  34:	42 40 00 03 	bdzla-  0 <start>
  38:	42 40 00 01 	bdzl-   38 <start\+0x38>
  3c:	41 82 00 00 	beq-    3c <start\+0x3c>
  40:	41 8a 00 02 	beqa-   2,0 <start>
  44:	41 86 00 01 	beql-   1,44 <start\+0x44>
  48:	41 8e 00 03 	beqla-  3,0 <start>
  4c:	40 80 00 00 	bge-    4c <start\+0x4c>
  50:	40 90 00 02 	bgea-   4,0 <start>
  54:	40 88 00 01 	bgel-   2,54 <start\+0x54>
  58:	40 98 00 03 	bgela-  6,0 <start>
  5c:	41 91 00 00 	bgt-    4,5c <start\+0x5c>
  60:	41 99 00 02 	bgta-   6,0 <start>
  64:	41 95 00 01 	bgtl-   5,64 <start\+0x64>
  68:	41 9d 00 03 	bgtla-  7,0 <start>
  6c:	48 00 00 00 	b       6c <start\+0x6c>
  70:	48 00 00 03 	bla     0 <start>
  74:	40 81 00 00 	ble-    74 <start\+0x74>
  78:	40 91 00 02 	blea-   4,0 <start>
  7c:	40 89 00 01 	blel-   2,7c <start\+0x7c>
  80:	40 99 00 03 	blela-  6,0 <start>
  84:	48 00 00 01 	bl      84 <start\+0x84>
  88:	41 80 00 00 	blt-    88 <start\+0x88>
  8c:	41 88 00 02 	blta-   2,0 <start>
  90:	41 84 00 01 	bltl-   1,90 <start\+0x90>
  94:	41 8c 00 03 	bltla-  3,0 <start>
  98:	40 82 00 00 	bne-    98 <start\+0x98>
  9c:	40 8a 00 02 	bnea-   2,0 <start>
  a0:	40 86 00 01 	bnel-   1,a0 <start\+0xa0>
  a4:	40 8e 00 03 	bnela-  3,0 <start>
  a8:	40 85 00 00 	ble-    1,a8 <start\+0xa8>
  ac:	40 95 00 02 	blea-   5,0 <start>
  b0:	40 8d 00 01 	blel-   3,b0 <start\+0xb0>
  b4:	40 9d 00 03 	blela-  7,0 <start>
  b8:	40 84 00 00 	bge-    1,b8 <start\+0xb8>
  bc:	40 94 00 02 	bgea-   5,0 <start>
  c0:	40 8c 00 01 	bgel-   3,c0 <start\+0xc0>
  c4:	40 9c 00 03 	bgela-  7,0 <start>
  c8:	40 93 00 00 	bns-    4,c8 <start\+0xc8>
  cc:	40 9b 00 02 	bnsa-   6,0 <start>
  d0:	40 97 00 01 	bnsl-   5,d0 <start\+0xd0>
  d4:	40 9f 00 03 	bnsla-  7,0 <start>
  d8:	41 93 00 00 	bso-    4,d8 <start\+0xd8>
  dc:	41 9b 00 02 	bsoa-   6,0 <start>
  e0:	41 97 00 01 	bsol-   5,e0 <start\+0xe0>
  e4:	41 9f 00 03 	bsola-  7,0 <start>
  e8:	4c 85 32 02 	crand   4,5,6
  ec:	4c 64 29 02 	crandc  3,4,5
  f0:	4c e0 0a 42 	creqv   7,0,1
  f4:	4c 22 19 c2 	crnand  1,2,3
  f8:	4c 01 10 42 	crnor   0,1,2
  fc:	4c a6 3b 82 	cror    5,6,7
 100:	4c 43 23 42 	crorc   2,3,4
 104:	4c c7 01 82 	crxor   6,7,0
 108:	7d 6a 62 39 	eqv.    r10,r11,r12
 10c:	7d 6a 62 38 	eqv     r10,r11,r12
 110:	fe a0 fa 11 	fabs.   f21,f31
 114:	fe a0 fa 10 	fabs    f21,f31
 118:	fd 8a 58 40 	fcmpo   3,f10,f11
 11c:	fd 84 28 00 	fcmpu   3,f4,f5
 120:	fc 60 20 91 	fmr.    f3,f4
 124:	fc 60 20 90 	fmr     f3,f4
 128:	fe 80 f1 11 	fnabs.  f20,f30
 12c:	fe 80 f1 10 	fnabs   f20,f30
 130:	fc 60 20 51 	fneg.   f3,f4
 134:	fc 60 20 50 	fneg    f3,f4
 138:	fc c0 38 18 	frsp    f6,f7
 13c:	fd 00 48 19 	frsp.   f8,f9
 140:	89 21 00 00 	lbz     r9,0\(r1\)
 144:	8d 41 00 01 	lbzu    r10,1\(r1\)
 148:	7e 95 b0 ee 	lbzux   r20,r21,r22
 14c:	7c 64 28 ae 	lbzx    r3,r4,r5
 150:	ca a1 00 08 	lfd     f21,8\(r1\)
 154:	ce c1 00 10 	lfdu    f22,16\(r1\)
 158:	7e 95 b4 ee 	lfdux   f20,r21,r22
 15c:	7d ae 7c ae 	lfdx    f13,r14,r15
 160:	c2 61 00 00 	lfs     f19,0\(r1\)
 164:	c6 81 00 04 	lfsu    f20,4\(r1\)
 168:	7d 4b 64 6e 	lfsux   f10,r11,r12
 16c:	7d 4b 64 2e 	lfsx    f10,r11,r12
 170:	a9 e1 00 06 	lha     r15,6\(r1\)
 174:	ae 01 00 08 	lhau    r16,8\(r1\)
 178:	7d 2a 5a ee 	lhaux   r9,r10,r11
 17c:	7d 2a 5a ae 	lhax    r9,r10,r11
 180:	7c 64 2e 2c 	lhbrx   r3,r4,r5
 184:	a1 a1 00 00 	lhz     r13,0\(r1\)
 188:	a5 c1 00 02 	lhzu    r14,2\(r1\)
 18c:	7e 96 c2 6e 	lhzux   r20,r22,r24
 190:	7e f8 ca 2e 	lhzx    r23,r24,r25
 194:	4c 04 00 00 	mcrf    0,1
 198:	fd 90 00 80 	mcrfs   3,4
 19c:	7d 80 04 00 	mcrxr   3
 1a0:	7c 60 00 26 	mfcr    r3
 1a4:	7c 69 02 a6 	mfctr   r3
 1a8:	7c b3 02 a6 	mfdar   r5
 1ac:	7c 92 02 a6 	mfdsisr r4
 1b0:	ff c0 04 8e 	mffs    f30
 1b4:	ff e0 04 8f 	mffs.   f31
 1b8:	7c 48 02 a6 	mflr    r2
 1bc:	7e 60 00 a6 	mfmsr   r19
 1c0:	7c 78 00 26 	mfocrf  r3,128
 1c4:	7c 25 02 a6 	mfrtcl  r1
 1c8:	7c 04 02 a6 	mfrtcu  r0
 1cc:	7c d9 02 a6 	mfsdr1  r6
 1d0:	7c 60 22 a6 	mfspr   r3,128
 1d4:	7c fa 02 a6 	mfsrr0  r7
 1d8:	7d 1b 02 a6 	mfsrr1  r8
 1dc:	7f c1 02 a6 	mfxer   r30
 1e0:	7f fe fb 79 	mr.     r30,r31
 1e4:	7f fe fb 78 	mr      r30,r31
 1e8:	7c 6f f1 20 	mtcr    r3
 1ec:	7c 68 01 20 	mtcrf   128,r3
 1f0:	7e 69 03 a6 	mtctr   r19
 1f4:	7e b3 03 a6 	mtdar   r21
 1f8:	7f 16 03 a6 	mtdec   r24
 1fc:	7e 92 03 a6 	mtdsisr r20
 200:	fc 60 00 8d 	mtfsb0. 3
 204:	fc 60 00 8c 	mtfsb0  3
 208:	fc 60 00 4d 	mtfsb1. 3
 20c:	fc 60 00 4c 	mtfsb1  3
 210:	fc 0c 55 8e 	mtfsf   6,f10
 214:	fc 0c 5d 8f 	mtfsf.  6,f11
 218:	ff 00 01 0c 	mtfsfi  6,0
 21c:	ff 00 f1 0d 	mtfsfi. 6,15
 220:	7e 48 03 a6 	mtlr    r18
 224:	7d 40 01 24 	mtmsr   r10
 228:	7c 78 01 20 	mtocrf  128,r3
 22c:	7e f5 03 a6 	mtrtcl  r23
 230:	7e d4 03 a6 	mtrtcu  r22
 234:	7f 39 03 a6 	mtsdr1  r25
 238:	7c 60 23 a6 	mtspr   128,r3
 23c:	7f 5a 03 a6 	mtsrr0  r26
 240:	7f 7b 03 a6 	mtsrr1  r27
 244:	7e 21 03 a6 	mtxer   r17
 248:	7f bc f3 b9 	nand.   r28,r29,r30
 24c:	7f bc f3 b8 	nand    r28,r29,r30
 250:	7c 64 00 d1 	neg.    r3,r4
 254:	7c 64 00 d0 	neg     r3,r4
 258:	7e 11 04 d0 	nego    r16,r17
 25c:	7e 53 04 d1 	nego.   r18,r19
 260:	7e b4 b0 f9 	nor.    r20,r21,r22
 264:	7e b4 b0 f8 	nor     r20,r21,r22
 268:	7e b4 a8 f9 	not.    r20,r21
 26c:	7e b4 a8 f8 	not     r20,r21
 270:	7c 40 23 78 	or      r0,r2,r4
 274:	7d cc 83 79 	or.     r12,r14,r16
 278:	7e 0f 8b 38 	orc     r15,r16,r17
 27c:	7e 72 a3 39 	orc.    r18,r19,r20
 280:	4c 00 00 64 	rfi
 284:	99 61 00 02 	stb     r11,2\(r1\)
 288:	9d 81 00 03 	stbu    r12,3\(r1\)
 28c:	7d ae 79 ee 	stbux   r13,r14,r15
 290:	7c 64 29 ae 	stbx    r3,r4,r5
 294:	db 21 00 20 	stfd    f25,32\(r1\)
 298:	df 41 00 28 	stfdu   f26,40\(r1\)
 29c:	7c 01 15 ee 	stfdux  f0,r1,r2
 2a0:	7f be fd ae 	stfdx   f29,r30,r31
 2a4:	d2 e1 00 14 	stfs    f23,20\(r1\)
 2a8:	d7 01 00 18 	stfsu   f24,24\(r1\)
 2ac:	7f 5b e5 6e 	stfsux  f26,r27,r28
 2b0:	7e f8 cd 2e 	stfsx   f23,r24,r25
 2b4:	b2 21 00 0a 	sth     r17,10\(r1\)
 2b8:	7c c7 47 2c 	sthbrx  r6,r7,r8
 2bc:	b6 41 00 0c 	sthu    r18,12\(r1\)
 2c0:	7e b6 bb 6e 	sthux   r21,r22,r23
 2c4:	7d 8d 73 2e 	sthx    r12,r13,r14
 2c8:	7f dd fa 79 	xor.    r29,r30,r31
 2cc:	7f dd fa 78 	xor     r29,r30,r31
