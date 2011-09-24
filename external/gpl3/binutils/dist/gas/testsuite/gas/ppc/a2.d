#as: -ma2
#objdump: -dr -Ma2
#name: A2 tests


.*: +file format elf(32)?(64)?-powerpc.*


Disassembly of section \.text:

0+00 <start>:
   0:	7c 85 32 15 	add\.    r4,r5,r6
   4:	7c 85 32 14 	add     r4,r5,r6
   8:	7c 85 30 15 	addc\.   r4,r5,r6
   c:	7c 85 30 14 	addc    r4,r5,r6
  10:	7c 85 34 15 	addco\.  r4,r5,r6
  14:	7c 85 34 14 	addco   r4,r5,r6
  18:	7c 85 31 15 	adde\.   r4,r5,r6
  1c:	7c 85 31 14 	adde    r4,r5,r6
  20:	7c 85 35 15 	addeo\.  r4,r5,r6
  24:	7c 85 35 14 	addeo   r4,r5,r6
  28:	38 85 00 0d 	addi    r4,r5,13
  2c:	38 85 ff f3 	addi    r4,r5,-13
  30:	34 85 00 0d 	addic\.  r4,r5,13
  34:	34 85 ff f3 	addic\.  r4,r5,-13
  38:	30 85 00 0d 	addic   r4,r5,13
  3c:	30 85 ff f3 	addic   r4,r5,-13
  40:	3c 85 00 17 	addis   r4,r5,23
  44:	3c 85 ff e9 	addis   r4,r5,-23
  48:	7c 85 01 d5 	addme\.  r4,r5
  4c:	7c 85 01 d4 	addme   r4,r5
  50:	7c 85 05 d5 	addmeo\. r4,r5
  54:	7c 85 05 d4 	addmeo  r4,r5
  58:	7c 85 36 15 	addo\.   r4,r5,r6
  5c:	7c 85 36 14 	addo    r4,r5,r6
  60:	7c 85 01 95 	addze\.  r4,r5
  64:	7c 85 01 94 	addze   r4,r5
  68:	7c 85 05 95 	addzeo\. r4,r5
  6c:	7c 85 05 94 	addzeo  r4,r5
  70:	7c a4 30 39 	and\.    r4,r5,r6
  74:	7c a4 30 38 	and     r4,r5,r6
  78:	7c a4 30 79 	andc\.   r4,r5,r6
  7c:	7c a4 30 78 	andc    r4,r5,r6
  80:	70 a4 00 06 	andi\.   r4,r5,6
  84:	74 a4 00 06 	andis\.  r4,r5,6
  88:	00 00 02 00 	attn
  8c:	48 00 00 02 	ba      0 <start>
			8c: R_PPC(|64)_ADDR24	label_abs
  90:	40 01 00 00 	bdnzf   gt,90 <start\+0x90>
			90: R_PPC(|64)_REL14	foo
  94:	40 01 00 00 	bdnzf   gt,94 <start\+0x94>
			94: R_PPC(|64)_REL14	foo
  98:	40 01 00 00 	bdnzf   gt,98 <start\+0x98>
			98: R_PPC(|64)_REL14	foo
  9c:	40 85 00 02 	blea    cr1,0 <start>
			9c: R_PPC(|64)_ADDR14	foo_abs
  a0:	40 c5 00 02 	blea-   cr1,0 <start>
			a0: R_PPC(|64)_ADDR14	foo_abs
  a4:	40 e5 00 02 	blea\+   cr1,0 <start>
			a4: R_PPC(|64)_ADDR14	foo_abs
  a8:	4c 86 0c 20 	bcctr   4,4\*cr1\+eq,1
  ac:	4c 86 04 20 	bnectr  cr1
  b0:	4c a6 04 20 	bcctr\+  4,4\*cr1\+eq
  b4:	4c 86 0c 21 	bcctrl  4,4\*cr1\+eq,1
  b8:	4c 86 04 21 	bnectrl cr1
  bc:	4c a6 04 21 	bcctrl\+ 4,4\*cr1\+eq
  c0:	40 01 00 01 	bdnzfl  gt,c0 <start\+0xc0>
			c0: R_PPC(|64)_REL14	foo
  c4:	40 01 00 01 	bdnzfl  gt,c4 <start\+0xc4>
			c4: R_PPC(|64)_REL14	foo
  c8:	40 01 00 01 	bdnzfl  gt,c8 <start\+0xc8>
			c8: R_PPC(|64)_REL14	foo
  cc:	40 85 00 03 	blela   cr1,0 <start>
			cc: R_PPC(|64)_ADDR14	foo_abs
  d0:	40 c5 00 03 	blela-  cr1,0 <start>
			d0: R_PPC(|64)_ADDR14	foo_abs
  d4:	40 e5 00 03 	blela\+  cr1,0 <start>
			d4: R_PPC(|64)_ADDR14	foo_abs
  d8:	4c 86 08 20 	bclr    4,4\*cr1\+eq,1
  dc:	4c 86 00 20 	bnelr   cr1
  e0:	4c a6 00 20 	bclr\+   4,4\*cr1\+eq
  e4:	4c 86 08 21 	bclrl   4,4\*cr1\+eq,1
  e8:	4c 86 00 21 	bnelrl  cr1
  ec:	4c a6 00 21 	bclrl\+  4,4\*cr1\+eq
  f0:	48 00 00 00 	b       f0 <start\+0xf0>
			f0: R_PPC(|64)_REL24	label
  f4:	48 00 00 03 	bla     0 <start>
			f4: R_PPC(|64)_ADDR24	label_abs
  f8:	48 00 00 01 	bl      f8 <start\+0xf8>
			f8: R_PPC(|64)_REL24	label
  fc:	7d 6a 61 f8 	bpermd  r10,r11,r12
 100:	7c a7 40 00 	cmpd    cr1,r7,r8
 104:	7d 6a 63 f8 	cmpb    r10,r11,r12
 108:	2c aa 00 0d 	cmpdi   cr1,r10,13
 10c:	2c aa ff f3 	cmpdi   cr1,r10,-13
 110:	7c a7 40 40 	cmpld   cr1,r7,r8
 114:	28 aa 00 64 	cmpldi  cr1,r10,100
 118:	7e b4 00 75 	cntlzd\. r20,r21
 11c:	7e b4 00 74 	cntlzd  r20,r21
 120:	7e b4 00 35 	cntlzw\. r20,r21
 124:	7e b4 00 34 	cntlzw  r20,r21
 128:	4c 22 1a 02 	crand   gt,eq,so
 12c:	4c 22 19 02 	crandc  gt,eq,so
 130:	4c 22 1a 42 	creqv   gt,eq,so
 134:	4c 22 19 c2 	crnand  gt,eq,so
 138:	4c 22 18 42 	crnor   gt,eq,so
 13c:	4c 22 1b 82 	cror    gt,eq,so
 140:	4c 22 1b 42 	crorc   gt,eq,so
 144:	4c 22 19 82 	crxor   gt,eq,so
 148:	7c 0a 5d ec 	dcba    r10,r11
 14c:	7c 0a 58 ac 	dcbf    r10,r11
 150:	7c 2a 58 ac 	dcbfl   r10,r11
 154:	7c 0a 58 fe 	dcbfep  r10,r11
 158:	7c 0a 5b ac 	dcbi    r10,r11
 15c:	7c 0a 5b 0c 	dcblc   r10,r11
 160:	7c 2a 5b 0c 	dcblc   1,r10,r11
 164:	7c 0a 58 6c 	dcbst   r10,r11
 168:	7c 0a 58 7e 	dcbstep r10,r11
 16c:	7c 0a 5a 2c 	dcbt    r10,r11
 170:	7c 2a 5a 2c 	dcbt    r10,r11,1
 174:	7d 4b 62 7e 	dcbtep  r10,r11,r12
 178:	7c 0a 59 4c 	dcbtls  r10,r11
 17c:	7c 2a 59 4c 	dcbtls  1,r10,r11
 180:	7c 0a 59 ec 	dcbtst  r10,r11
 184:	7c 2a 59 ec 	dcbtst  r10,r11,1
 188:	7d 4b 61 fe 	dcbtstep r10,r11,r12
 18c:	7c 0a 59 0c 	dcbtstls r10,r11
 190:	7c 2a 59 0c 	dcbtstls 1,r10,r11
 194:	7c 0a 5f ec 	dcbz    r10,r11
 198:	7c 0a 5f fe 	dcbzep  r10,r11
 19c:	7c 00 03 8c 	dccci   
 1a0:	7c 00 03 8c 	dccci   
 1a4:	7c 00 03 8c 	dccci   
 1a8:	7d 40 03 8c 	dci     10
 1ac:	7e 95 b3 d3 	divd\.   r20,r21,r22
 1b0:	7e 95 b3 d2 	divd    r20,r21,r22
 1b4:	7e 95 b7 d3 	divdo\.  r20,r21,r22
 1b8:	7e 95 b7 d2 	divdo   r20,r21,r22
 1bc:	7e 95 b3 93 	divdu\.  r20,r21,r22
 1c0:	7e 95 b3 92 	divdu   r20,r21,r22
 1c4:	7e 95 b7 93 	divduo\. r20,r21,r22
 1c8:	7e 95 b7 92 	divduo  r20,r21,r22
 1cc:	7e 95 b3 d7 	divw\.   r20,r21,r22
 1d0:	7e 95 b3 d6 	divw    r20,r21,r22
 1d4:	7e 95 b7 d7 	divwo\.  r20,r21,r22
 1d8:	7e 95 b7 d6 	divwo   r20,r21,r22
 1dc:	7e 95 b3 97 	divwu\.  r20,r21,r22
 1e0:	7e 95 b3 96 	divwu   r20,r21,r22
 1e4:	7e 95 b7 97 	divwuo\. r20,r21,r22
 1e8:	7e 95 b7 96 	divwuo  r20,r21,r22
 1ec:	7e b4 b2 39 	eqv\.    r20,r21,r22
 1f0:	7e b4 b2 38 	eqv     r20,r21,r22
 1f4:	7c 0a 58 66 	eratilx 0,r10,r11
 1f8:	7c 2a 58 66 	eratilx 1,r10,r11
 1fc:	7c ea 58 66 	eratilx 7,r10,r11
 200:	7d 4b 66 66 	erativax r10,r11,r12
 204:	7d 4b 01 66 	eratre  r10,r11,0
 208:	7d 4b 19 66 	eratre  r10,r11,3
 20c:	7d 4b 61 27 	eratsx\. r10,r11,r12
 210:	7d 4b 61 26 	eratsx  r10,r11,r12
 214:	7d 4b 01 a6 	eratwe  r10,r11,0
 218:	7d 4b 19 a6 	eratwe  r10,r11,3
 21c:	7d 6a 07 75 	extsb\.  r10,r11
 220:	7d 6a 07 74 	extsb   r10,r11
 224:	7d 6a 07 35 	extsh\.  r10,r11
 228:	7d 6a 07 34 	extsh   r10,r11
 22c:	7d 6a 07 b5 	extsw\.  r10,r11
 230:	7d 6a 07 b4 	extsw   r10,r11
 234:	fe 80 aa 11 	fabs\.   f20,f21
 238:	fe 80 aa 10 	fabs    f20,f21
 23c:	fe 95 b0 2b 	fadd\.   f20,f21,f22
 240:	fe 95 b0 2a 	fadd    f20,f21,f22
 244:	ee 95 b0 2b 	fadds\.  f20,f21,f22
 248:	ee 95 b0 2a 	fadds   f20,f21,f22
 24c:	fe 80 ae 9d 	fcfid\.  f20,f21
 250:	fe 80 ae 9c 	fcfid   f20,f21
 254:	fc 14 a8 40 	fcmpo   cr0,f20,f21
 258:	fc 94 a8 40 	fcmpo   cr1,f20,f21
 25c:	fc 14 a8 00 	fcmpu   cr0,f20,f21
 260:	fc 94 a8 00 	fcmpu   cr1,f20,f21
 264:	fe 95 b0 11 	fcpsgn\. f20,f21,f22
 268:	fe 95 b0 10 	fcpsgn  f20,f21,f22
 26c:	fe 80 ae 5d 	fctid\.  f20,f21
 270:	fe 80 ae 5c 	fctid   f20,f21
 274:	fe 80 ae 5f 	fctidz\. f20,f21
 278:	fe 80 ae 5e 	fctidz  f20,f21
 27c:	fe 80 a8 1d 	fctiw\.  f20,f21
 280:	fe 80 a8 1c 	fctiw   f20,f21
 284:	fe 80 a8 1f 	fctiwz\. f20,f21
 288:	fe 80 a8 1e 	fctiwz  f20,f21
 28c:	fe 95 b0 25 	fdiv\.   f20,f21,f22
 290:	fe 95 b0 24 	fdiv    f20,f21,f22
 294:	ee 95 b0 25 	fdivs\.  f20,f21,f22
 298:	ee 95 b0 24 	fdivs   f20,f21,f22
 29c:	fe 95 bd bb 	fmadd\.  f20,f21,f22,f23
 2a0:	fe 95 bd ba 	fmadd   f20,f21,f22,f23
 2a4:	ee 95 bd bb 	fmadds\. f20,f21,f22,f23
 2a8:	ee 95 bd ba 	fmadds  f20,f21,f22,f23
 2ac:	fe 80 a8 91 	fmr\.    f20,f21
 2b0:	fe 80 a8 90 	fmr     f20,f21
 2b4:	fe 95 bd b9 	fmsub\.  f20,f21,f22,f23
 2b8:	fe 95 bd b8 	fmsub   f20,f21,f22,f23
 2bc:	ee 95 bd b9 	fmsubs\. f20,f21,f22,f23
 2c0:	ee 95 bd b8 	fmsubs  f20,f21,f22,f23
 2c4:	fe 95 05 b3 	fmul\.   f20,f21,f22
 2c8:	fe 95 05 b2 	fmul    f20,f21,f22
 2cc:	ee 95 05 b3 	fmuls\.  f20,f21,f22
 2d0:	ee 95 05 b2 	fmuls   f20,f21,f22
 2d4:	fe 80 a9 11 	fnabs\.  f20,f21
 2d8:	fe 80 a9 10 	fnabs   f20,f21
 2dc:	fe 80 a8 51 	fneg\.   f20,f21
 2e0:	fe 80 a8 50 	fneg    f20,f21
 2e4:	fe 95 bd bf 	fnmadd\. f20,f21,f22,f23
 2e8:	fe 95 bd be 	fnmadd  f20,f21,f22,f23
 2ec:	ee 95 bd bf 	fnmadds\. f20,f21,f22,f23
 2f0:	ee 95 bd be 	fnmadds f20,f21,f22,f23
 2f4:	fe 95 bd bd 	fnmsub\. f20,f21,f22,f23
 2f8:	fe 95 bd bc 	fnmsub  f20,f21,f22,f23
 2fc:	ee 95 bd bd 	fnmsubs\. f20,f21,f22,f23
 300:	ee 95 bd bc 	fnmsubs f20,f21,f22,f23
 304:	fe 80 a8 31 	fre\.    f20,f21
 308:	fe 80 a8 30 	fre     f20,f21
 30c:	fe 80 a8 31 	fre\.    f20,f21
 310:	fe 80 a8 30 	fre     f20,f21
 314:	fe 81 a8 31 	fre\.    f20,f21,1
 318:	fe 81 a8 30 	fre     f20,f21,1
 31c:	ee 80 a8 31 	fres\.   f20,f21
 320:	ee 80 a8 30 	fres    f20,f21
 324:	ee 80 a8 31 	fres\.   f20,f21
 328:	ee 80 a8 30 	fres    f20,f21
 32c:	ee 81 a8 31 	fres\.   f20,f21,1
 330:	ee 81 a8 30 	fres    f20,f21,1
 334:	fe 80 ab d1 	frim\.   f20,f21
 338:	fe 80 ab d0 	frim    f20,f21
 33c:	fe 80 ab 11 	frin\.   f20,f21
 340:	fe 80 ab 10 	frin    f20,f21
 344:	fe 80 ab 91 	frip\.   f20,f21
 348:	fe 80 ab 90 	frip    f20,f21
 34c:	fe 80 ab 51 	friz\.   f20,f21
 350:	fe 80 ab 50 	friz    f20,f21
 354:	fe 80 a8 19 	frsp\.   f20,f21
 358:	fe 80 a8 18 	frsp    f20,f21
 35c:	fe 80 a8 35 	frsqrte\. f20,f21
 360:	fe 80 a8 34 	frsqrte f20,f21
 364:	fe 80 a8 35 	frsqrte\. f20,f21
 368:	fe 80 a8 34 	frsqrte f20,f21
 36c:	fe 81 a8 35 	frsqrte\. f20,f21,1
 370:	fe 81 a8 34 	frsqrte f20,f21,1
 374:	ee 80 a8 34 	frsqrtes f20,f21
 378:	ee 80 a8 35 	frsqrtes\. f20,f21
 37c:	ee 80 a8 34 	frsqrtes f20,f21
 380:	ee 80 a8 35 	frsqrtes\. f20,f21
 384:	ee 81 a8 34 	frsqrtes f20,f21,1
 388:	ee 81 a8 35 	frsqrtes\. f20,f21,1
 38c:	fe 95 bd af 	fsel\.   f20,f21,f22,f23
 390:	fe 95 bd ae 	fsel    f20,f21,f22,f23
 394:	fe 80 a8 2d 	fsqrt\.  f20,f21
 398:	fe 80 a8 2c 	fsqrt   f20,f21
 39c:	ee 80 a8 2d 	fsqrts\. f20,f21
 3a0:	ee 80 a8 2c 	fsqrts  f20,f21
 3a4:	fe 95 b0 29 	fsub\.   f20,f21,f22
 3a8:	fe 95 b0 28 	fsub    f20,f21,f22
 3ac:	ee 95 b0 29 	fsubs\.  f20,f21,f22
 3b0:	ee 95 b0 28 	fsubs   f20,f21,f22
 3b4:	7c 0a 5f ac 	icbi    r10,r11
 3b8:	7c 0a 5f be 	icbiep  r10,r11
 3bc:	7c 0a 58 2c 	icbt    r10,r11
 3c0:	7c ea 58 2c 	icbt    7,r10,r11
 3c4:	7c 0a 5b cc 	icbtls  r10,r11
 3c8:	7c ea 5b cc 	icbtls  7,r10,r11
 3cc:	7c 00 07 8c 	iccci   
 3d0:	7c 00 07 8c 	iccci   
 3d4:	7c 00 07 8c 	iccci   
 3d8:	7d 40 07 8c 	ici     10
 3dc:	7d 4b 63 2d 	icswx\.  r10,r11,r12
 3e0:	7d 4b 63 2c 	icswx   r10,r11,r12
 3e4:	7d 4b 65 de 	isel    r10,r11,r12,23
 3e8:	4c 00 01 2c 	isync
 3ec:	7d 4b 60 be 	lbepx   r10,r11,r12
 3f0:	89 4b ff ef 	lbz     r10,-17\(r11\)
 3f4:	89 4b 00 11 	lbz     r10,17\(r11\)
 3f8:	8d 4b ff ff 	lbzu    r10,-1\(r11\)
 3fc:	8d 4b 00 01 	lbzu    r10,1\(r11\)
 400:	7d 4b 68 ee 	lbzux   r10,r11,r13
 404:	7d 4b 68 ae 	lbzx    r10,r11,r13
 408:	e9 4b ff f8 	ld      r10,-8\(r11\)
 40c:	e9 4b 00 08 	ld      r10,8\(r11\)
 410:	7d 4b 60 a8 	ldarx   r10,r11,r12
 414:	7d 4b 60 a9 	ldarx   r10,r11,r12,1
 418:	7d 4b 64 28 	ldbrx   r10,r11,r12
 41c:	7d 4b 60 3a 	ldepx   r10,r11,r12
 420:	e9 4b ff f9 	ldu     r10,-8\(r11\)
 424:	e9 4b 00 09 	ldu     r10,8\(r11\)
 428:	7d 4b 60 6a 	ldux    r10,r11,r12
 42c:	7d 4b 60 2a 	ldx     r10,r11,r12
 430:	ca 8a ff f8 	lfd     f20,-8\(r10\)
 434:	ca 8a 00 08 	lfd     f20,8\(r10\)
 438:	7e 8a 5c be 	lfdepx  f20,r10,r11
 43c:	ce 8a ff f8 	lfdu    f20,-8\(r10\)
 440:	ce 8a 00 08 	lfdu    f20,8\(r10\)
 444:	7e 8a 5c ee 	lfdux   f20,r10,r11
 448:	7e 8a 5c ae 	lfdx    f20,r10,r11
 44c:	7e 8a 5e ae 	lfiwax  f20,r10,r11
 450:	7e 8a 5e ee 	lfiwzx  f20,r10,r11
 454:	c2 8a ff fc 	lfs     f20,-4\(r10\)
 458:	c2 8a 00 04 	lfs     f20,4\(r10\)
 45c:	c6 8a ff fc 	lfsu    f20,-4\(r10\)
 460:	c6 8a 00 04 	lfsu    f20,4\(r10\)
 464:	7e 8a 5c 6e 	lfsux   f20,r10,r11
 468:	7e 8a 5c 2e 	lfsx    f20,r10,r11
 46c:	a9 4b 00 02 	lha     r10,2\(r11\)
 470:	ad 4b ff fe 	lhau    r10,-2\(r11\)
 474:	7d 4b 62 ee 	lhaux   r10,r11,r12
 478:	7d 4b 62 ae 	lhax    r10,r11,r12
 47c:	7d 4b 66 2c 	lhbrx   r10,r11,r12
 480:	7d 4b 62 3e 	lhepx   r10,r11,r12
 484:	a1 4b ff fe 	lhz     r10,-2\(r11\)
 488:	a1 4b 00 02 	lhz     r10,2\(r11\)
 48c:	a5 4b ff fe 	lhzu    r10,-2\(r11\)
 490:	a5 4b 00 02 	lhzu    r10,2\(r11\)
 494:	7d 4b 62 6e 	lhzux   r10,r11,r12
 498:	7d 4b 62 2e 	lhzx    r10,r11,r12
 49c:	ba 8a 00 10 	lmw     r20,16\(r10\)
 4a0:	7d 4b 0c aa 	lswi    r10,r11,1
 4a4:	7d 4b 04 aa 	lswi    r10,r11,32
 4a8:	7d 4b 64 2a 	lswx    r10,r11,r12
 4ac:	e9 4b ff fe 	lwa     r10,-4\(r11\)
 4b0:	e9 4b 00 06 	lwa     r10,4\(r11\)
 4b4:	7d 4b 60 28 	lwarx   r10,r11,r12
 4b8:	7d 4b 60 29 	lwarx   r10,r11,r12,1
 4bc:	7d 4b 62 ea 	lwaux   r10,r11,r12
 4c0:	7d 4b 62 aa 	lwax    r10,r11,r12
 4c4:	7d 4b 64 2c 	lwbrx   r10,r11,r12
 4c8:	7d 4b 60 3e 	lwepx   r10,r11,r12
 4cc:	81 4b ff fc 	lwz     r10,-4\(r11\)
 4d0:	81 4b 00 04 	lwz     r10,4\(r11\)
 4d4:	85 4b ff fc 	lwzu    r10,-4\(r11\)
 4d8:	85 4b 00 04 	lwzu    r10,4\(r11\)
 4dc:	7d 4b 60 6e 	lwzux   r10,r11,r12
 4e0:	7d 4b 60 2e 	lwzx    r10,r11,r12
 4e4:	7c 00 06 ac 	mbar    
 4e8:	7c 00 06 ac 	mbar    
 4ec:	7c 00 06 ac 	mbar    
 4f0:	7c 20 06 ac 	mbar    1
 4f4:	4c 04 00 00 	mcrf    cr0,cr1
 4f8:	fd 90 00 80 	mcrfs   cr3,cr4
 4fc:	7c 00 04 00 	mcrxr   cr0
 500:	7d 80 04 00 	mcrxr   cr3
 504:	7c 60 00 26 	mfcr    r3
 508:	7c 60 00 26 	mfcr    r3
 50c:	7c 70 10 26 	mfocrf  r3,1
 510:	7c 78 00 26 	mfocrf  r3,128
 514:	7d 4a 3a 87 	mfdcr\.  r10,234
 518:	7d 4a 3a 86 	mfdcr   r10,234
 51c:	7d 4b 02 07 	mfdcrx\. r10,r11
 520:	7d 4b 02 06 	mfdcrx  r10,r11
 524:	fe 80 04 8f 	mffs\.   f20
 528:	fe 80 04 8e 	mffs    f20
 52c:	7d 40 00 a6 	mfmsr   r10
 530:	7c 70 10 26 	mfocrf  r3,1
 534:	7c 78 00 26 	mfocrf  r3,128
 538:	7d 4a 3a a6 	mfspr   r10,234
 53c:	7d 4c 42 e6 	mftbl   r10
 540:	7d 4d 42 e6 	mftbu   r10
 544:	7c 00 51 dc 	msgclr  r10
 548:	7c 00 51 9c 	msgsnd  r10
 54c:	7c 60 01 20 	mtcrf   0,r3
 550:	7c 70 11 20 	mtocrf  1,r3
 554:	7c 78 01 20 	mtocrf  128,r3
 558:	7c 6f f1 20 	mtcr    r3
 55c:	7d 4a 3b 87 	mtdcr\.  234,r10
 560:	7d 4a 3b 86 	mtdcr   234,r10
 564:	7d 6a 03 07 	mtdcrx\. r10,r11
 568:	7d 6a 03 06 	mtdcrx  r10,r11
 56c:	fc 60 00 8d 	mtfsb0\. so
 570:	fc 60 00 8c 	mtfsb0  so
 574:	fc 60 00 4d 	mtfsb1\. so
 578:	fc 60 00 4c 	mtfsb1  so
 57c:	fc 0c a5 8f 	mtfsf\.  6,f20
 580:	fc 0c a5 8e 	mtfsf   6,f20
 584:	fc 0c a5 8f 	mtfsf\.  6,f20
 588:	fc 0c a5 8e 	mtfsf   6,f20
 58c:	fe 0d a5 8f 	mtfsf\.  6,f20,1,1
 590:	fe 0d a5 8e 	mtfsf   6,f20,1,1
 594:	ff 00 01 0d 	mtfsfi\. 6,0
 598:	ff 00 01 0c 	mtfsfi  6,0
 59c:	ff 00 d1 0d 	mtfsfi\. 6,13
 5a0:	ff 00 d1 0c 	mtfsfi  6,13
 5a4:	ff 01 d1 0d 	mtfsfi\. 6,13,1
 5a8:	ff 01 d1 0c 	mtfsfi  6,13,1
 5ac:	7d 40 01 24 	mtmsr   r10
 5b0:	7d 40 01 24 	mtmsr   r10
 5b4:	7d 41 01 24 	mtmsr   r10,1
 5b8:	7c 70 11 20 	mtocrf  1,r3
 5bc:	7c 78 01 20 	mtocrf  128,r3
 5c0:	7d 4a 3b a6 	mtspr   234,r10
 5c4:	7e 95 b0 93 	mulhd\.  r20,r21,r22
 5c8:	7e 95 b0 92 	mulhd   r20,r21,r22
 5cc:	7e 95 b0 13 	mulhdu\. r20,r21,r22
 5d0:	7e 95 b0 12 	mulhdu  r20,r21,r22
 5d4:	7e 95 b0 97 	mulhw\.  r20,r21,r22
 5d8:	7e 95 b0 96 	mulhw   r20,r21,r22
 5dc:	7e 95 b0 17 	mulhwu\. r20,r21,r22
 5e0:	7e 95 b0 16 	mulhwu  r20,r21,r22
 5e4:	7e 95 b1 d3 	mulld\.  r20,r21,r22
 5e8:	7e 95 b1 d2 	mulld   r20,r21,r22
 5ec:	7e 95 b5 d3 	mulldo\. r20,r21,r22
 5f0:	7e 95 b5 d2 	mulldo  r20,r21,r22
 5f4:	1e 95 00 64 	mulli   r20,r21,100
 5f8:	1e 95 ff 9c 	mulli   r20,r21,-100
 5fc:	7e 95 b1 d7 	mullw\.  r20,r21,r22
 600:	7e 95 b1 d6 	mullw   r20,r21,r22
 604:	7e 95 b5 d7 	mullwo\. r20,r21,r22
 608:	7e 95 b5 d6 	mullwo  r20,r21,r22
 60c:	7e b4 b3 b9 	nand\.   r20,r21,r22
 610:	7e b4 b3 b8 	nand    r20,r21,r22
 614:	7e 95 00 d1 	neg\.    r20,r21
 618:	7e 95 00 d0 	neg     r20,r21
 61c:	7e 95 04 d1 	nego\.   r20,r21
 620:	7e 95 04 d0 	nego    r20,r21
 624:	7e b4 b0 f9 	nor\.    r20,r21,r22
 628:	7e b4 b0 f8 	nor     r20,r21,r22
 62c:	7e b4 b3 79 	or\.     r20,r21,r22
 630:	7e b4 b3 78 	or      r20,r21,r22
 634:	7e b4 b3 39 	orc\.    r20,r21,r22
 638:	7e b4 b3 38 	orc     r20,r21,r22
 63c:	62 b4 10 00 	ori     r20,r21,4096
 640:	66 b4 10 00 	oris    r20,r21,4096
 644:	7d 6a 00 f4 	popcntb r10,r11
 648:	7d 6a 03 f4 	popcntd r10,r11
 64c:	7d 6a 02 f4 	popcntw r10,r11
 650:	7d 6a 01 74 	prtyd   r10,r11
 654:	7d 6a 01 34 	prtyw   r10,r11
 658:	4c 00 00 66 	rfci
 65c:	4c 00 00 cc 	rfgi
 660:	4c 00 00 64 	rfi
 664:	4c 00 00 4c 	rfmci
 668:	79 6a 67 f1 	rldcl\.  r10,r11,r12,63
 66c:	79 6a 67 f0 	rldcl   r10,r11,r12,63
 670:	79 6a 67 f3 	rldcr\.  r10,r11,r12,63
 674:	79 6a 67 f2 	rldcr   r10,r11,r12,63
 678:	79 6a bf e9 	rldic\.  r10,r11,23,63
 67c:	79 6a bf e8 	rldic   r10,r11,23,63
 680:	79 6a bf e1 	rldicl\. r10,r11,23,63
 684:	79 6a bf e0 	rldicl  r10,r11,23,63
 688:	79 6a bf e5 	rldicr\. r10,r11,23,63
 68c:	79 6a bf e4 	rldicr  r10,r11,23,63
 690:	79 6a bf ed 	rldimi\. r10,r11,23,63
 694:	79 6a bf ec 	rldimi  r10,r11,23,63
 698:	51 6a b8 3f 	rlwimi\. r10,r11,23,0,31
 69c:	51 6a b8 3e 	rlwimi  r10,r11,23,0,31
 6a0:	55 6a b8 3f 	rotlwi\. r10,r11,23
 6a4:	55 6a b8 3e 	rotlwi  r10,r11,23
 6a8:	5d 6a b8 3f 	rotlw\.  r10,r11,r23
 6ac:	5d 6a b8 3e 	rotlw   r10,r11,r23
 6b0:	44 00 00 02 	sc      
 6b4:	44 00 0c 82 	sc      100
 6b8:	7d 6a 60 37 	sld\.    r10,r11,r12
 6bc:	7d 6a 60 36 	sld     r10,r11,r12
 6c0:	7d 6a 60 31 	slw\.    r10,r11,r12
 6c4:	7d 6a 60 30 	slw     r10,r11,r12
 6c8:	7d 6a 66 35 	srad\.   r10,r11,r12
 6cc:	7d 6a 66 34 	srad    r10,r11,r12
 6d0:	7d 6a fe 77 	sradi\.  r10,r11,63
 6d4:	7d 6a fe 76 	sradi   r10,r11,63
 6d8:	7d 6a 66 31 	sraw\.   r10,r11,r12
 6dc:	7d 6a 66 30 	sraw    r10,r11,r12
 6e0:	7d 6a fe 71 	srawi\.  r10,r11,31
 6e4:	7d 6a fe 70 	srawi   r10,r11,31
 6e8:	7d 6a 64 37 	srd\.    r10,r11,r12
 6ec:	7d 6a 64 36 	srd     r10,r11,r12
 6f0:	7d 6a 64 31 	srw\.    r10,r11,r12
 6f4:	7d 6a 64 30 	srw     r10,r11,r12
 6f8:	99 4b ff ff 	stb     r10,-1\(r11\)
 6fc:	99 4b 00 01 	stb     r10,1\(r11\)
 700:	7d 4b 61 be 	stbepx  r10,r11,r12
 704:	9d 4b ff ff 	stbu    r10,-1\(r11\)
 708:	9d 4b 00 01 	stbu    r10,1\(r11\)
 70c:	7d 4b 61 ee 	stbux   r10,r11,r12
 710:	7d 4b 61 ae 	stbx    r10,r11,r12
 714:	f9 4b ff f8 	std     r10,-8\(r11\)
 718:	f9 4b 00 08 	std     r10,8\(r11\)
 71c:	7d 4b 65 28 	stdbrx  r10,r11,r12
 720:	7d 4b 61 ad 	stdcx\.  r10,r11,r12
 724:	7d 4b 61 3a 	stdepx  r10,r11,r12
 728:	f9 4b ff f9 	stdu    r10,-8\(r11\)
 72c:	f9 4b 00 09 	stdu    r10,8\(r11\)
 730:	7d 4b 61 6a 	stdux   r10,r11,r12
 734:	7d 4b 61 2a 	stdx    r10,r11,r12
 738:	da 8a ff f8 	stfd    f20,-8\(r10\)
 73c:	da 8a 00 08 	stfd    f20,8\(r10\)
 740:	7e 8a 5d be 	stfdepx f20,r10,r11
 744:	de 8a ff f8 	stfdu   f20,-8\(r10\)
 748:	de 8a 00 08 	stfdu   f20,8\(r10\)
 74c:	7e 8a 5d ee 	stfdux  f20,r10,r11
 750:	7e 8a 5d ae 	stfdx   f20,r10,r11
 754:	7e 8a 5f ae 	stfiwx  f20,r10,r11
 758:	d2 8a ff fc 	stfs    f20,-4\(r10\)
 75c:	d2 8a 00 04 	stfs    f20,4\(r10\)
 760:	d6 8a ff fc 	stfsu   f20,-4\(r10\)
 764:	d6 8a 00 04 	stfsu   f20,4\(r10\)
 768:	7e 8a 5d 6e 	stfsux  f20,r10,r11
 76c:	7e 8a 5d 2e 	stfsx   f20,r10,r11
 770:	b1 4b ff fe 	sth     r10,-2\(r11\)
 774:	b1 4b 00 02 	sth     r10,2\(r11\)
 778:	b1 4b ff fc 	sth     r10,-4\(r11\)
 77c:	b1 4b 00 04 	sth     r10,4\(r11\)
 780:	7d 4b 67 2c 	sthbrx  r10,r11,r12
 784:	7d 4b 63 3e 	sthepx  r10,r11,r12
 788:	b5 4b ff fe 	sthu    r10,-2\(r11\)
 78c:	b5 4b 00 02 	sthu    r10,2\(r11\)
 790:	7d 4b 63 6e 	sthux   r10,r11,r12
 794:	7d 4b 63 2e 	sthx    r10,r11,r12
 798:	be 8a 00 10 	stmw    r20,16\(r10\)
 79c:	7d 4b 0d aa 	stswi   r10,r11,1
 7a0:	7d 4b 05 aa 	stswi   r10,r11,32
 7a4:	7d 4b 65 2a 	stswx   r10,r11,r12
 7a8:	7d 4b 65 2c 	stwbrx  r10,r11,r12
 7ac:	7d 4b 61 2d 	stwcx\.  r10,r11,r12
 7b0:	7d 4b 61 3e 	stwepx  r10,r11,r12
 7b4:	95 4b ff fc 	stwu    r10,-4\(r11\)
 7b8:	95 4b 00 04 	stwu    r10,4\(r11\)
 7bc:	7d 4b 61 6e 	stwux   r10,r11,r12
 7c0:	7d 4b 61 2e 	stwx    r10,r11,r12
 7c4:	7e 95 b0 51 	subf\.   r20,r21,r22
 7c8:	7e 95 b0 50 	subf    r20,r21,r22
 7cc:	7e 95 b0 11 	subfc\.  r20,r21,r22
 7d0:	7e 95 b0 10 	subfc   r20,r21,r22
 7d4:	7e 95 b4 11 	subfco\. r20,r21,r22
 7d8:	7e 95 b4 10 	subfco  r20,r21,r22
 7dc:	7e 95 b1 11 	subfe\.  r20,r21,r22
 7e0:	7e 95 b1 10 	subfe   r20,r21,r22
 7e4:	7e 95 b5 11 	subfeo\. r20,r21,r22
 7e8:	7e 95 b5 10 	subfeo  r20,r21,r22
 7ec:	22 95 00 64 	subfic  r20,r21,100
 7f0:	22 95 ff 9c 	subfic  r20,r21,-100
 7f4:	7e 95 01 d1 	subfme\. r20,r21
 7f8:	7e 95 01 d0 	subfme  r20,r21
 7fc:	7e 95 05 d1 	subfmeo\. r20,r21
 800:	7e 95 05 d0 	subfmeo r20,r21
 804:	7e 95 b4 51 	subfo\.  r20,r21,r22
 808:	7e 95 b4 50 	subfo   r20,r21,r22
 80c:	7e 95 01 91 	subfze\. r20,r21
 810:	7e 95 01 90 	subfze  r20,r21
 814:	7e 95 05 91 	subfzeo\. r20,r21
 818:	7e 95 05 90 	subfzeo r20,r21
 81c:	7c 00 04 ac 	sync    
 820:	7c 00 04 ac 	sync    
 824:	7c 00 04 ac 	sync    
 828:	7c 20 04 ac 	lwsync
 82c:	7c aa 58 88 	tdlge   r10,r11
 830:	08 aa 00 64 	tdlgei  r10,100
 834:	08 aa ff 9c 	tdlgei  r10,-100
 838:	7c 6a 58 24 	tlbilxva r10,r11
 83c:	7c 0a 5e 24 	tlbivax r10,r11
 840:	7c 00 07 64 	tlbre   
 844:	7d 4b 3f 64 	tlbre   r10,r11,7
 848:	7c 0a 5e a5 	tlbsrx\. r10,r11
 84c:	7d 4b 67 25 	tlbsx\.  r10,r11,r12
 850:	7d 4b 67 24 	tlbsx   r10,r11,r12
 854:	7c 00 04 6c 	tlbsync
 858:	7c 00 07 a4 	tlbwe   
 85c:	7d 4b 3f a4 	tlbwe   r10,r11,7
 860:	7c aa 58 08 	twlge   r10,r11
 864:	0c aa 00 64 	twlgei  r10,100
 868:	0c aa ff 9c 	twlgei  r10,-100
 86c:	7c 00 00 7c 	wait    
 870:	7c 00 00 7c 	wait    
 874:	7c 20 00 7c 	waitrsv
 878:	7c 40 00 7c 	waitimpl
 87c:	7c 40 00 7c 	waitimpl
 880:	7c 20 00 7c 	waitrsv
 884:	7c 00 01 6c 	wchkall 
 888:	7c 00 01 6c 	wchkall 
 88c:	7d 80 01 6c 	wchkall cr3
 890:	7c 2a 5f 4c 	wclr    1,r10,r11
 894:	7c 20 07 4c 	wclrall 1
 898:	7c 4a 5f 4c 	wclrone r10,r11
 89c:	7d 40 01 06 	wrtee   r10
 8a0:	7c 00 81 46 	wrteei  1
 8a4:	7d 6a 62 79 	xor\.    r10,r11,r12
 8a8:	7d 6a 62 78 	xor     r10,r11,r12
 8ac:	69 6a 10 00 	xori    r10,r11,4096
 8b0:	6d 6a 10 00 	xoris   r10,r11,4096
