#as: -ma2
#objdump: -dr -Ma2
#name: A2 tests


.*


Disassembly of section \.text:

0+00 <start>:
   0:	(7c 85 32 15|15 32 85 7c) 	add\.    r4,r5,r6
   4:	(7c 85 32 14|14 32 85 7c) 	add     r4,r5,r6
   8:	(7c 85 30 15|15 30 85 7c) 	addc\.   r4,r5,r6
   c:	(7c 85 30 14|14 30 85 7c) 	addc    r4,r5,r6
  10:	(7c 85 34 15|15 34 85 7c) 	addco\.  r4,r5,r6
  14:	(7c 85 34 14|14 34 85 7c) 	addco   r4,r5,r6
  18:	(7c 85 31 15|15 31 85 7c) 	adde\.   r4,r5,r6
  1c:	(7c 85 31 14|14 31 85 7c) 	adde    r4,r5,r6
  20:	(7c 85 35 15|15 35 85 7c) 	addeo\.  r4,r5,r6
  24:	(7c 85 35 14|14 35 85 7c) 	addeo   r4,r5,r6
  28:	(38 85 00 0d|0d 00 85 38) 	addi    r4,r5,13
  2c:	(38 85 ff f3|f3 ff 85 38) 	addi    r4,r5,-13
  30:	(34 85 00 0d|0d 00 85 34) 	addic\.  r4,r5,13
  34:	(34 85 ff f3|f3 ff 85 34) 	addic\.  r4,r5,-13
  38:	(30 85 00 0d|0d 00 85 30) 	addic   r4,r5,13
  3c:	(30 85 ff f3|f3 ff 85 30) 	addic   r4,r5,-13
  40:	(3c 85 00 17|17 00 85 3c) 	addis   r4,r5,23
  44:	(3c 85 ff e9|e9 ff 85 3c) 	addis   r4,r5,-23
  48:	(7c 85 01 d5|d5 01 85 7c) 	addme\.  r4,r5
  4c:	(7c 85 01 d4|d4 01 85 7c) 	addme   r4,r5
  50:	(7c 85 05 d5|d5 05 85 7c) 	addmeo\. r4,r5
  54:	(7c 85 05 d4|d4 05 85 7c) 	addmeo  r4,r5
  58:	(7c 85 36 15|15 36 85 7c) 	addo\.   r4,r5,r6
  5c:	(7c 85 36 14|14 36 85 7c) 	addo    r4,r5,r6
  60:	(7c 85 01 95|95 01 85 7c) 	addze\.  r4,r5
  64:	(7c 85 01 94|94 01 85 7c) 	addze   r4,r5
  68:	(7c 85 05 95|95 05 85 7c) 	addzeo\. r4,r5
  6c:	(7c 85 05 94|94 05 85 7c) 	addzeo  r4,r5
  70:	(7c a4 30 39|39 30 a4 7c) 	and\.    r4,r5,r6
  74:	(7c a4 30 38|38 30 a4 7c) 	and     r4,r5,r6
  78:	(7c a4 30 79|79 30 a4 7c) 	andc\.   r4,r5,r6
  7c:	(7c a4 30 78|78 30 a4 7c) 	andc    r4,r5,r6
  80:	(70 a4 00 06|06 00 a4 70) 	andi\.   r4,r5,6
  84:	(74 a4 00 06|06 00 a4 74) 	andis\.  r4,r5,6
  88:	(00 00 02 00|00 02 00 00) 	attn
  8c:	(48 00 00 02|02 00 00 48) 	ba      0 <start>
			8c: R_PPC(|64)_ADDR24	label_abs
  90:	(40 01 00 00|00 00 01 40) 	bdnzf   gt,90 <start\+0x90>
			90: R_PPC(|64)_REL14	foo
  94:	(40 01 00 00|00 00 01 40) 	bdnzf   gt,94 <start\+0x94>
			94: R_PPC(|64)_REL14	foo
  98:	(40 01 00 00|00 00 01 40) 	bdnzf   gt,98 <start\+0x98>
			98: R_PPC(|64)_REL14	foo
  9c:	(40 85 00 02|02 00 85 40) 	blea    cr1,0 <start>
			9c: R_PPC(|64)_ADDR14	foo_abs
  a0:	(40 c5 00 02|02 00 c5 40) 	blea-   cr1,0 <start>
			a0: R_PPC(|64)_ADDR14	foo_abs
  a4:	(40 e5 00 02|02 00 e5 40) 	blea\+   cr1,0 <start>
			a4: R_PPC(|64)_ADDR14	foo_abs
  a8:	(4c 86 0c 20|20 0c 86 4c) 	bcctr   4,4\*cr1\+eq,1
  ac:	(4c 86 04 20|20 04 86 4c) 	bnectr  cr1
  b0:	(4c a6 04 20|20 04 a6 4c) 	bcctr\+  4,4\*cr1\+eq
  b4:	(4c 86 0c 21|21 0c 86 4c) 	bcctrl  4,4\*cr1\+eq,1
  b8:	(4c 86 04 21|21 04 86 4c) 	bnectrl cr1
  bc:	(4c a6 04 21|21 04 a6 4c) 	bcctrl\+ 4,4\*cr1\+eq
  c0:	(40 01 00 01|01 00 01 40) 	bdnzfl  gt,c0 <start\+0xc0>
			c0: R_PPC(|64)_REL14	foo
  c4:	(40 01 00 01|01 00 01 40) 	bdnzfl  gt,c4 <start\+0xc4>
			c4: R_PPC(|64)_REL14	foo
  c8:	(40 01 00 01|01 00 01 40) 	bdnzfl  gt,c8 <start\+0xc8>
			c8: R_PPC(|64)_REL14	foo
  cc:	(40 85 00 03|03 00 85 40) 	blela   cr1,0 <start>
			cc: R_PPC(|64)_ADDR14	foo_abs
  d0:	(40 c5 00 03|03 00 c5 40) 	blela-  cr1,0 <start>
			d0: R_PPC(|64)_ADDR14	foo_abs
  d4:	(40 e5 00 03|03 00 e5 40) 	blela\+  cr1,0 <start>
			d4: R_PPC(|64)_ADDR14	foo_abs
  d8:	(4c 86 08 20|20 08 86 4c) 	bclr    4,4\*cr1\+eq,1
  dc:	(4c 86 00 20|20 00 86 4c) 	bnelr   cr1
  e0:	(4c a6 00 20|20 00 a6 4c) 	bclr\+   4,4\*cr1\+eq
  e4:	(4c 86 08 21|21 08 86 4c) 	bclrl   4,4\*cr1\+eq,1
  e8:	(4c 86 00 21|21 00 86 4c) 	bnelrl  cr1
  ec:	(4c a6 00 21|21 00 a6 4c) 	bclrl\+  4,4\*cr1\+eq
  f0:	(48 00 00 00|00 00 00 48) 	b       f0 <start\+0xf0>
			f0: R_PPC(|64)_REL24	label
  f4:	(48 00 00 03|03 00 00 48) 	bla     0 <start>
			f4: R_PPC(|64)_ADDR24	label_abs
  f8:	(48 00 00 01|01 00 00 48) 	bl      f8 <start\+0xf8>
			f8: R_PPC(|64)_REL24	label
  fc:	(7d 6a 61 f8|f8 61 6a 7d) 	bpermd  r10,r11,r12
 100:	(7c a7 40 00|00 40 a7 7c) 	cmpd    cr1,r7,r8
 104:	(7d 6a 63 f8|f8 63 6a 7d) 	cmpb    r10,r11,r12
 108:	(2c aa 00 0d|0d 00 aa 2c) 	cmpdi   cr1,r10,13
 10c:	(2c aa ff f3|f3 ff aa 2c) 	cmpdi   cr1,r10,-13
 110:	(7c a7 40 40|40 40 a7 7c) 	cmpld   cr1,r7,r8
 114:	(28 aa 00 64|64 00 aa 28) 	cmpldi  cr1,r10,100
 118:	(7e b4 00 75|75 00 b4 7e) 	cntlzd\. r20,r21
 11c:	(7e b4 00 74|74 00 b4 7e) 	cntlzd  r20,r21
 120:	(7e b4 00 35|35 00 b4 7e) 	cntlzw\. r20,r21
 124:	(7e b4 00 34|34 00 b4 7e) 	cntlzw  r20,r21
 128:	(4c 22 1a 02|02 1a 22 4c) 	crand   gt,eq,so
 12c:	(4c 22 19 02|02 19 22 4c) 	crandc  gt,eq,so
 130:	(4c 22 1a 42|42 1a 22 4c) 	creqv   gt,eq,so
 134:	(4c 22 19 c2|c2 19 22 4c) 	crnand  gt,eq,so
 138:	(4c 22 18 42|42 18 22 4c) 	crnor   gt,eq,so
 13c:	(4c 22 1b 82|82 1b 22 4c) 	cror    gt,eq,so
 140:	(4c 22 1b 42|42 1b 22 4c) 	crorc   gt,eq,so
 144:	(4c 22 19 82|82 19 22 4c) 	crxor   gt,eq,so
 148:	(7c 0a 5d ec|ec 5d 0a 7c) 	dcba    r10,r11
 14c:	(7c 0a 58 ac|ac 58 0a 7c) 	dcbf    r10,r11
 150:	(7c 2a 58 ac|ac 58 2a 7c) 	dcbfl   r10,r11
 154:	(7c 0a 58 fe|fe 58 0a 7c) 	dcbfep  r10,r11
 158:	(7c 0a 5b ac|ac 5b 0a 7c) 	dcbi    r10,r11
 15c:	(7c 0a 5b 0c|0c 5b 0a 7c) 	dcblc   r10,r11
 160:	(7c 2a 5b 0c|0c 5b 2a 7c) 	dcblc   1,r10,r11
 164:	(7c 0a 58 6c|6c 58 0a 7c) 	dcbst   r10,r11
 168:	(7c 0a 58 7e|7e 58 0a 7c) 	dcbstep r10,r11
 16c:	(7c 0a 5a 2c|2c 5a 0a 7c) 	dcbt    r10,r11
 170:	(7c 2a 5a 2c|2c 5a 2a 7c) 	dcbt    r10,r11,1
 174:	(7d 4b 62 7e|7e 62 4b 7d) 	dcbtep  r10,r11,r12
 178:	(7c 0a 59 4c|4c 59 0a 7c) 	dcbtls  r10,r11
 17c:	(7c 2a 59 4c|4c 59 2a 7c) 	dcbtls  1,r10,r11
 180:	(7c 0a 59 ec|ec 59 0a 7c) 	dcbtst  r10,r11
 184:	(7c 2a 59 ec|ec 59 2a 7c) 	dcbtst  r10,r11,1
 188:	(7d 4b 61 fe|fe 61 4b 7d) 	dcbtstep r10,r11,r12
 18c:	(7c 0a 59 0c|0c 59 0a 7c) 	dcbtstls r10,r11
 190:	(7c 2a 59 0c|0c 59 2a 7c) 	dcbtstls 1,r10,r11
 194:	(7c 0a 5f ec|ec 5f 0a 7c) 	dcbz    r10,r11
 198:	(7c 0a 5f fe|fe 5f 0a 7c) 	dcbzep  r10,r11
 19c:	(7c 00 03 8c|8c 03 00 7c) 	dccci   
 1a0:	(7c 00 03 8c|8c 03 00 7c) 	dccci   
 1a4:	(7c 00 03 8c|8c 03 00 7c) 	dccci   
 1a8:	(7d 40 03 8c|8c 03 40 7d) 	dci     10
 1ac:	(7e 95 b3 d3|d3 b3 95 7e) 	divd\.   r20,r21,r22
 1b0:	(7e 95 b3 d2|d2 b3 95 7e) 	divd    r20,r21,r22
 1b4:	(7e 95 b7 d3|d3 b7 95 7e) 	divdo\.  r20,r21,r22
 1b8:	(7e 95 b7 d2|d2 b7 95 7e) 	divdo   r20,r21,r22
 1bc:	(7e 95 b3 93|93 b3 95 7e) 	divdu\.  r20,r21,r22
 1c0:	(7e 95 b3 92|92 b3 95 7e) 	divdu   r20,r21,r22
 1c4:	(7e 95 b7 93|93 b7 95 7e) 	divduo\. r20,r21,r22
 1c8:	(7e 95 b7 92|92 b7 95 7e) 	divduo  r20,r21,r22
 1cc:	(7e 95 b3 d7|d7 b3 95 7e) 	divw\.   r20,r21,r22
 1d0:	(7e 95 b3 d6|d6 b3 95 7e) 	divw    r20,r21,r22
 1d4:	(7e 95 b7 d7|d7 b7 95 7e) 	divwo\.  r20,r21,r22
 1d8:	(7e 95 b7 d6|d6 b7 95 7e) 	divwo   r20,r21,r22
 1dc:	(7e 95 b3 97|97 b3 95 7e) 	divwu\.  r20,r21,r22
 1e0:	(7e 95 b3 96|96 b3 95 7e) 	divwu   r20,r21,r22
 1e4:	(7e 95 b7 97|97 b7 95 7e) 	divwuo\. r20,r21,r22
 1e8:	(7e 95 b7 96|96 b7 95 7e) 	divwuo  r20,r21,r22
 1ec:	(7e b4 b2 39|39 b2 b4 7e) 	eqv\.    r20,r21,r22
 1f0:	(7e b4 b2 38|38 b2 b4 7e) 	eqv     r20,r21,r22
 1f4:	(7c 0a 58 66|66 58 0a 7c) 	eratilx 0,r10,r11
 1f8:	(7c 2a 58 66|66 58 2a 7c) 	eratilx 1,r10,r11
 1fc:	(7c ea 58 66|66 58 ea 7c) 	eratilx 7,r10,r11
 200:	(7d 4b 66 66|66 66 4b 7d) 	erativax r10,r11,r12
 204:	(7d 4b 01 66|66 01 4b 7d) 	eratre  r10,r11,0
 208:	(7d 4b 19 66|66 19 4b 7d) 	eratre  r10,r11,3
 20c:	(7d 4b 61 27|27 61 4b 7d) 	eratsx\. r10,r11,r12
 210:	(7d 4b 61 26|26 61 4b 7d) 	eratsx  r10,r11,r12
 214:	(7d 4b 01 a6|a6 01 4b 7d) 	eratwe  r10,r11,0
 218:	(7d 4b 19 a6|a6 19 4b 7d) 	eratwe  r10,r11,3
 21c:	(7d 6a 07 75|75 07 6a 7d) 	extsb\.  r10,r11
 220:	(7d 6a 07 74|74 07 6a 7d) 	extsb   r10,r11
 224:	(7d 6a 07 35|35 07 6a 7d) 	extsh\.  r10,r11
 228:	(7d 6a 07 34|34 07 6a 7d) 	extsh   r10,r11
 22c:	(7d 6a 07 b5|b5 07 6a 7d) 	extsw\.  r10,r11
 230:	(7d 6a 07 b4|b4 07 6a 7d) 	extsw   r10,r11
 234:	(fe 80 aa 11|11 aa 80 fe) 	fabs\.   f20,f21
 238:	(fe 80 aa 10|10 aa 80 fe) 	fabs    f20,f21
 23c:	(fe 95 b0 2b|2b b0 95 fe) 	fadd\.   f20,f21,f22
 240:	(fe 95 b0 2a|2a b0 95 fe) 	fadd    f20,f21,f22
 244:	(ee 95 b0 2b|2b b0 95 ee) 	fadds\.  f20,f21,f22
 248:	(ee 95 b0 2a|2a b0 95 ee) 	fadds   f20,f21,f22
 24c:	(fe 80 ae 9d|9d ae 80 fe) 	fcfid\.  f20,f21
 250:	(fe 80 ae 9c|9c ae 80 fe) 	fcfid   f20,f21
 254:	(fc 14 a8 40|40 a8 14 fc) 	fcmpo   cr0,f20,f21
 258:	(fc 94 a8 40|40 a8 94 fc) 	fcmpo   cr1,f20,f21
 25c:	(fc 14 a8 00|00 a8 14 fc) 	fcmpu   cr0,f20,f21
 260:	(fc 94 a8 00|00 a8 94 fc) 	fcmpu   cr1,f20,f21
 264:	(fe 95 b0 11|11 b0 95 fe) 	fcpsgn\. f20,f21,f22
 268:	(fe 95 b0 10|10 b0 95 fe) 	fcpsgn  f20,f21,f22
 26c:	(fe 80 ae 5d|5d ae 80 fe) 	fctid\.  f20,f21
 270:	(fe 80 ae 5c|5c ae 80 fe) 	fctid   f20,f21
 274:	(fe 80 ae 5f|5f ae 80 fe) 	fctidz\. f20,f21
 278:	(fe 80 ae 5e|5e ae 80 fe) 	fctidz  f20,f21
 27c:	(fe 80 a8 1d|1d a8 80 fe) 	fctiw\.  f20,f21
 280:	(fe 80 a8 1c|1c a8 80 fe) 	fctiw   f20,f21
 284:	(fe 80 a8 1f|1f a8 80 fe) 	fctiwz\. f20,f21
 288:	(fe 80 a8 1e|1e a8 80 fe) 	fctiwz  f20,f21
 28c:	(fe 95 b0 25|25 b0 95 fe) 	fdiv\.   f20,f21,f22
 290:	(fe 95 b0 24|24 b0 95 fe) 	fdiv    f20,f21,f22
 294:	(ee 95 b0 25|25 b0 95 ee) 	fdivs\.  f20,f21,f22
 298:	(ee 95 b0 24|24 b0 95 ee) 	fdivs   f20,f21,f22
 29c:	(fe 95 bd bb|bb bd 95 fe) 	fmadd\.  f20,f21,f22,f23
 2a0:	(fe 95 bd ba|ba bd 95 fe) 	fmadd   f20,f21,f22,f23
 2a4:	(ee 95 bd bb|bb bd 95 ee) 	fmadds\. f20,f21,f22,f23
 2a8:	(ee 95 bd ba|ba bd 95 ee) 	fmadds  f20,f21,f22,f23
 2ac:	(fe 80 a8 91|91 a8 80 fe) 	fmr\.    f20,f21
 2b0:	(fe 80 a8 90|90 a8 80 fe) 	fmr     f20,f21
 2b4:	(fe 95 bd b9|b9 bd 95 fe) 	fmsub\.  f20,f21,f22,f23
 2b8:	(fe 95 bd b8|b8 bd 95 fe) 	fmsub   f20,f21,f22,f23
 2bc:	(ee 95 bd b9|b9 bd 95 ee) 	fmsubs\. f20,f21,f22,f23
 2c0:	(ee 95 bd b8|b8 bd 95 ee) 	fmsubs  f20,f21,f22,f23
 2c4:	(fe 95 05 b3|b3 05 95 fe) 	fmul\.   f20,f21,f22
 2c8:	(fe 95 05 b2|b2 05 95 fe) 	fmul    f20,f21,f22
 2cc:	(ee 95 05 b3|b3 05 95 ee) 	fmuls\.  f20,f21,f22
 2d0:	(ee 95 05 b2|b2 05 95 ee) 	fmuls   f20,f21,f22
 2d4:	(fe 80 a9 11|11 a9 80 fe) 	fnabs\.  f20,f21
 2d8:	(fe 80 a9 10|10 a9 80 fe) 	fnabs   f20,f21
 2dc:	(fe 80 a8 51|51 a8 80 fe) 	fneg\.   f20,f21
 2e0:	(fe 80 a8 50|50 a8 80 fe) 	fneg    f20,f21
 2e4:	(fe 95 bd bf|bf bd 95 fe) 	fnmadd\. f20,f21,f22,f23
 2e8:	(fe 95 bd be|be bd 95 fe) 	fnmadd  f20,f21,f22,f23
 2ec:	(ee 95 bd bf|bf bd 95 ee) 	fnmadds\. f20,f21,f22,f23
 2f0:	(ee 95 bd be|be bd 95 ee) 	fnmadds f20,f21,f22,f23
 2f4:	(fe 95 bd bd|bd bd 95 fe) 	fnmsub\. f20,f21,f22,f23
 2f8:	(fe 95 bd bc|bc bd 95 fe) 	fnmsub  f20,f21,f22,f23
 2fc:	(ee 95 bd bd|bd bd 95 ee) 	fnmsubs\. f20,f21,f22,f23
 300:	(ee 95 bd bc|bc bd 95 ee) 	fnmsubs f20,f21,f22,f23
 304:	(fe 80 a8 31|31 a8 80 fe) 	fre\.    f20,f21
 308:	(fe 80 a8 30|30 a8 80 fe) 	fre     f20,f21
 30c:	(fe 80 a8 31|31 a8 80 fe) 	fre\.    f20,f21
 310:	(fe 80 a8 30|30 a8 80 fe) 	fre     f20,f21
 314:	(fe 81 a8 31|31 a8 81 fe) 	fre\.    f20,f21,1
 318:	(fe 81 a8 30|30 a8 81 fe) 	fre     f20,f21,1
 31c:	(ee 80 a8 31|31 a8 80 ee) 	fres\.   f20,f21
 320:	(ee 80 a8 30|30 a8 80 ee) 	fres    f20,f21
 324:	(ee 80 a8 31|31 a8 80 ee) 	fres\.   f20,f21
 328:	(ee 80 a8 30|30 a8 80 ee) 	fres    f20,f21
 32c:	(ee 81 a8 31|31 a8 81 ee) 	fres\.   f20,f21,1
 330:	(ee 81 a8 30|30 a8 81 ee) 	fres    f20,f21,1
 334:	(fe 80 ab d1|d1 ab 80 fe) 	frim\.   f20,f21
 338:	(fe 80 ab d0|d0 ab 80 fe) 	frim    f20,f21
 33c:	(fe 80 ab 11|11 ab 80 fe) 	frin\.   f20,f21
 340:	(fe 80 ab 10|10 ab 80 fe) 	frin    f20,f21
 344:	(fe 80 ab 91|91 ab 80 fe) 	frip\.   f20,f21
 348:	(fe 80 ab 90|90 ab 80 fe) 	frip    f20,f21
 34c:	(fe 80 ab 51|51 ab 80 fe) 	friz\.   f20,f21
 350:	(fe 80 ab 50|50 ab 80 fe) 	friz    f20,f21
 354:	(fe 80 a8 19|19 a8 80 fe) 	frsp\.   f20,f21
 358:	(fe 80 a8 18|18 a8 80 fe) 	frsp    f20,f21
 35c:	(fe 80 a8 35|35 a8 80 fe) 	frsqrte\. f20,f21
 360:	(fe 80 a8 34|34 a8 80 fe) 	frsqrte f20,f21
 364:	(fe 80 a8 35|35 a8 80 fe) 	frsqrte\. f20,f21
 368:	(fe 80 a8 34|34 a8 80 fe) 	frsqrte f20,f21
 36c:	(fe 81 a8 35|35 a8 81 fe) 	frsqrte\. f20,f21,1
 370:	(fe 81 a8 34|34 a8 81 fe) 	frsqrte f20,f21,1
 374:	(ee 80 a8 34|34 a8 80 ee) 	frsqrtes f20,f21
 378:	(ee 80 a8 35|35 a8 80 ee) 	frsqrtes\. f20,f21
 37c:	(ee 80 a8 34|34 a8 80 ee) 	frsqrtes f20,f21
 380:	(ee 80 a8 35|35 a8 80 ee) 	frsqrtes\. f20,f21
 384:	(ee 81 a8 34|34 a8 81 ee) 	frsqrtes f20,f21,1
 388:	(ee 81 a8 35|35 a8 81 ee) 	frsqrtes\. f20,f21,1
 38c:	(fe 95 bd af|af bd 95 fe) 	fsel\.   f20,f21,f22,f23
 390:	(fe 95 bd ae|ae bd 95 fe) 	fsel    f20,f21,f22,f23
 394:	(fe 80 a8 2d|2d a8 80 fe) 	fsqrt\.  f20,f21
 398:	(fe 80 a8 2c|2c a8 80 fe) 	fsqrt   f20,f21
 39c:	(ee 80 a8 2d|2d a8 80 ee) 	fsqrts\. f20,f21
 3a0:	(ee 80 a8 2c|2c a8 80 ee) 	fsqrts  f20,f21
 3a4:	(fe 95 b0 29|29 b0 95 fe) 	fsub\.   f20,f21,f22
 3a8:	(fe 95 b0 28|28 b0 95 fe) 	fsub    f20,f21,f22
 3ac:	(ee 95 b0 29|29 b0 95 ee) 	fsubs\.  f20,f21,f22
 3b0:	(ee 95 b0 28|28 b0 95 ee) 	fsubs   f20,f21,f22
 3b4:	(7c 0a 5f ac|ac 5f 0a 7c) 	icbi    r10,r11
 3b8:	(7c 0a 5f be|be 5f 0a 7c) 	icbiep  r10,r11
 3bc:	(7c 0a 58 2c|2c 58 0a 7c) 	icbt    r10,r11
 3c0:	(7c ea 58 2c|2c 58 ea 7c) 	icbt    7,r10,r11
 3c4:	(7c 0a 5b cc|cc 5b 0a 7c) 	icbtls  r10,r11
 3c8:	(7c ea 5b cc|cc 5b ea 7c) 	icbtls  7,r10,r11
 3cc:	(7c 00 07 8c|8c 07 00 7c) 	iccci   
 3d0:	(7c 00 07 8c|8c 07 00 7c) 	iccci   
 3d4:	(7c 00 07 8c|8c 07 00 7c) 	iccci   
 3d8:	(7d 40 07 8c|8c 07 40 7d) 	ici     10
 3dc:	(7d 4b 63 2d|2d 63 4b 7d) 	icswx\.  r10,r11,r12
 3e0:	(7d 4b 63 2c|2c 63 4b 7d) 	icswx   r10,r11,r12
 3e4:	(7d 4b 65 de|de 65 4b 7d) 	isel    r10,r11,r12,23
 3e8:	(4c 00 01 2c|2c 01 00 4c) 	isync
 3ec:	(7d 4b 60 be|be 60 4b 7d) 	lbepx   r10,r11,r12
 3f0:	(89 4b ff ef|ef ff 4b 89) 	lbz     r10,-17\(r11\)
 3f4:	(89 4b 00 11|11 00 4b 89) 	lbz     r10,17\(r11\)
 3f8:	(8d 4b ff ff|ff ff 4b 8d) 	lbzu    r10,-1\(r11\)
 3fc:	(8d 4b 00 01|01 00 4b 8d) 	lbzu    r10,1\(r11\)
 400:	(7d 4b 68 ee|ee 68 4b 7d) 	lbzux   r10,r11,r13
 404:	(7d 4b 68 ae|ae 68 4b 7d) 	lbzx    r10,r11,r13
 408:	(e9 4b ff f8|f8 ff 4b e9) 	ld      r10,-8\(r11\)
 40c:	(e9 4b 00 08|08 00 4b e9) 	ld      r10,8\(r11\)
 410:	(7d 4b 60 a8|a8 60 4b 7d) 	ldarx   r10,r11,r12
 414:	(7d 4b 60 a9|a9 60 4b 7d) 	ldarx   r10,r11,r12,1
 418:	(7d 4b 64 28|28 64 4b 7d) 	ldbrx   r10,r11,r12
 41c:	(7d 4b 60 3a|3a 60 4b 7d) 	ldepx   r10,r11,r12
 420:	(e9 4b ff f9|f9 ff 4b e9) 	ldu     r10,-8\(r11\)
 424:	(e9 4b 00 09|09 00 4b e9) 	ldu     r10,8\(r11\)
 428:	(7d 4b 60 6a|6a 60 4b 7d) 	ldux    r10,r11,r12
 42c:	(7d 4b 60 2a|2a 60 4b 7d) 	ldx     r10,r11,r12
 430:	(ca 8a ff f8|f8 ff 8a ca) 	lfd     f20,-8\(r10\)
 434:	(ca 8a 00 08|08 00 8a ca) 	lfd     f20,8\(r10\)
 438:	(7e 8a 5c be|be 5c 8a 7e) 	lfdepx  f20,r10,r11
 43c:	(ce 8a ff f8|f8 ff 8a ce) 	lfdu    f20,-8\(r10\)
 440:	(ce 8a 00 08|08 00 8a ce) 	lfdu    f20,8\(r10\)
 444:	(7e 8a 5c ee|ee 5c 8a 7e) 	lfdux   f20,r10,r11
 448:	(7e 8a 5c ae|ae 5c 8a 7e) 	lfdx    f20,r10,r11
 44c:	(7e 8a 5e ae|ae 5e 8a 7e) 	lfiwax  f20,r10,r11
 450:	(7e 8a 5e ee|ee 5e 8a 7e) 	lfiwzx  f20,r10,r11
 454:	(c2 8a ff fc|fc ff 8a c2) 	lfs     f20,-4\(r10\)
 458:	(c2 8a 00 04|04 00 8a c2) 	lfs     f20,4\(r10\)
 45c:	(c6 8a ff fc|fc ff 8a c6) 	lfsu    f20,-4\(r10\)
 460:	(c6 8a 00 04|04 00 8a c6) 	lfsu    f20,4\(r10\)
 464:	(7e 8a 5c 6e|6e 5c 8a 7e) 	lfsux   f20,r10,r11
 468:	(7e 8a 5c 2e|2e 5c 8a 7e) 	lfsx    f20,r10,r11
 46c:	(a9 4b 00 02|02 00 4b a9) 	lha     r10,2\(r11\)
 470:	(ad 4b ff fe|fe ff 4b ad) 	lhau    r10,-2\(r11\)
 474:	(7d 4b 62 ee|ee 62 4b 7d) 	lhaux   r10,r11,r12
 478:	(7d 4b 62 ae|ae 62 4b 7d) 	lhax    r10,r11,r12
 47c:	(7d 4b 66 2c|2c 66 4b 7d) 	lhbrx   r10,r11,r12
 480:	(7d 4b 62 3e|3e 62 4b 7d) 	lhepx   r10,r11,r12
 484:	(a1 4b ff fe|fe ff 4b a1) 	lhz     r10,-2\(r11\)
 488:	(a1 4b 00 02|02 00 4b a1) 	lhz     r10,2\(r11\)
 48c:	(a5 4b ff fe|fe ff 4b a5) 	lhzu    r10,-2\(r11\)
 490:	(a5 4b 00 02|02 00 4b a5) 	lhzu    r10,2\(r11\)
 494:	(7d 4b 62 6e|6e 62 4b 7d) 	lhzux   r10,r11,r12
 498:	(7d 4b 62 2e|2e 62 4b 7d) 	lhzx    r10,r11,r12
 49c:	(ba 8a 00 10|10 00 8a ba) 	lmw     r20,16\(r10\)
 4a0:	(7d 4b 0c aa|aa 0c 4b 7d) 	lswi    r10,r11,1
 4a4:	(7d 8b 04 aa|aa 04 8b 7d) 	lswi    r12,r11,32
 4a8:	(7d 4b 64 2a|2a 64 4b 7d) 	lswx    r10,r11,r12
 4ac:	(e9 4b ff fe|fe ff 4b e9) 	lwa     r10,-4\(r11\)
 4b0:	(e9 4b 00 06|06 00 4b e9) 	lwa     r10,4\(r11\)
 4b4:	(7d 4b 60 28|28 60 4b 7d) 	lwarx   r10,r11,r12
 4b8:	(7d 4b 60 29|29 60 4b 7d) 	lwarx   r10,r11,r12,1
 4bc:	(7d 4b 62 ea|ea 62 4b 7d) 	lwaux   r10,r11,r12
 4c0:	(7d 4b 62 aa|aa 62 4b 7d) 	lwax    r10,r11,r12
 4c4:	(7d 4b 64 2c|2c 64 4b 7d) 	lwbrx   r10,r11,r12
 4c8:	(7d 4b 60 3e|3e 60 4b 7d) 	lwepx   r10,r11,r12
 4cc:	(81 4b ff fc|fc ff 4b 81) 	lwz     r10,-4\(r11\)
 4d0:	(81 4b 00 04|04 00 4b 81) 	lwz     r10,4\(r11\)
 4d4:	(85 4b ff fc|fc ff 4b 85) 	lwzu    r10,-4\(r11\)
 4d8:	(85 4b 00 04|04 00 4b 85) 	lwzu    r10,4\(r11\)
 4dc:	(7d 4b 60 6e|6e 60 4b 7d) 	lwzux   r10,r11,r12
 4e0:	(7d 4b 60 2e|2e 60 4b 7d) 	lwzx    r10,r11,r12
 4e4:	(7c 00 06 ac|ac 06 00 7c) 	mbar    
 4e8:	(7c 00 06 ac|ac 06 00 7c) 	mbar    
 4ec:	(7c 00 06 ac|ac 06 00 7c) 	mbar    
 4f0:	(7c 20 06 ac|ac 06 20 7c) 	mbar    1
 4f4:	(4c 04 00 00|00 00 04 4c) 	mcrf    cr0,cr1
 4f8:	(fd 90 00 80|80 00 90 fd) 	mcrfs   cr3,cr4
 4fc:	(7c 00 04 00|00 04 00 7c) 	mcrxr   cr0
 500:	(7d 80 04 00|00 04 80 7d) 	mcrxr   cr3
 504:	(7c 60 00 26|26 00 60 7c) 	mfcr    r3
 508:	(7c 60 00 26|26 00 60 7c) 	mfcr    r3
 50c:	(7c 70 10 26|26 10 70 7c) 	mfocrf  r3,1
 510:	(7c 78 00 26|26 00 78 7c) 	mfocrf  r3,128
 514:	(7d 4a 3a 87|87 3a 4a 7d) 	mfdcr\.  r10,234
 518:	(7d 4a 3a 86|86 3a 4a 7d) 	mfdcr   r10,234
 51c:	(7d 4b 02 07|07 02 4b 7d) 	mfdcrx\. r10,r11
 520:	(7d 4b 02 06|06 02 4b 7d) 	mfdcrx  r10,r11
 524:	(fe 80 04 8f|8f 04 80 fe) 	mffs\.   f20
 528:	(fe 80 04 8e|8e 04 80 fe) 	mffs    f20
 52c:	(7d 40 00 a6|a6 00 40 7d) 	mfmsr   r10
 530:	(7c 70 10 26|26 10 70 7c) 	mfocrf  r3,1
 534:	(7c 78 00 26|26 00 78 7c) 	mfocrf  r3,128
 538:	(7d 4a 3a a6|a6 3a 4a 7d) 	mfspr   r10,234
 53c:	(7d 4c 42 e6|e6 42 4c 7d) 	mftbl   r10
 540:	(7d 4d 42 e6|e6 42 4d 7d) 	mftbu   r10
 544:	(7c 00 51 dc|dc 51 00 7c) 	msgclr  r10
 548:	(7c 00 51 9c|9c 51 00 7c) 	msgsnd  r10
 54c:	(7c 60 01 20|20 01 60 7c) 	mtcrf   0,r3
 550:	(7c 70 11 20|20 11 70 7c) 	mtocrf  1,r3
 554:	(7c 78 01 20|20 01 78 7c) 	mtocrf  128,r3
 558:	(7c 6f f1 20|20 f1 6f 7c) 	mtcr    r3
 55c:	(7d 4a 3b 87|87 3b 4a 7d) 	mtdcr\.  234,r10
 560:	(7d 4a 3b 86|86 3b 4a 7d) 	mtdcr   234,r10
 564:	(7d 6a 03 07|07 03 6a 7d) 	mtdcrx\. r10,r11
 568:	(7d 6a 03 06|06 03 6a 7d) 	mtdcrx  r10,r11
 56c:	(fc 60 00 8d|8d 00 60 fc) 	mtfsb0\. so
 570:	(fc 60 00 8c|8c 00 60 fc) 	mtfsb0  so
 574:	(fc 60 00 4d|4d 00 60 fc) 	mtfsb1\. so
 578:	(fc 60 00 4c|4c 00 60 fc) 	mtfsb1  so
 57c:	(fc 0c a5 8f|8f a5 0c fc) 	mtfsf\.  6,f20
 580:	(fc 0c a5 8e|8e a5 0c fc) 	mtfsf   6,f20
 584:	(fc 0c a5 8f|8f a5 0c fc) 	mtfsf\.  6,f20
 588:	(fc 0c a5 8e|8e a5 0c fc) 	mtfsf   6,f20
 58c:	(fe 0d a5 8f|8f a5 0d fe) 	mtfsf\.  6,f20,1,1
 590:	(fe 0d a5 8e|8e a5 0d fe) 	mtfsf   6,f20,1,1
 594:	(ff 00 01 0d|0d 01 00 ff) 	mtfsfi\. 6,0
 598:	(ff 00 01 0c|0c 01 00 ff) 	mtfsfi  6,0
 59c:	(ff 00 d1 0d|0d d1 00 ff) 	mtfsfi\. 6,13
 5a0:	(ff 00 d1 0c|0c d1 00 ff) 	mtfsfi  6,13
 5a4:	(ff 01 d1 0d|0d d1 01 ff) 	mtfsfi\. 6,13,1
 5a8:	(ff 01 d1 0c|0c d1 01 ff) 	mtfsfi  6,13,1
 5ac:	(7d 40 01 24|24 01 40 7d) 	mtmsr   r10
 5b0:	(7d 40 01 24|24 01 40 7d) 	mtmsr   r10
 5b4:	(7d 41 01 24|24 01 41 7d) 	mtmsr   r10,1
 5b8:	(7c 70 11 20|20 11 70 7c) 	mtocrf  1,r3
 5bc:	(7c 78 01 20|20 01 78 7c) 	mtocrf  128,r3
 5c0:	(7d 4a 3b a6|a6 3b 4a 7d) 	mtspr   234,r10
 5c4:	(7e 95 b0 93|93 b0 95 7e) 	mulhd\.  r20,r21,r22
 5c8:	(7e 95 b0 92|92 b0 95 7e) 	mulhd   r20,r21,r22
 5cc:	(7e 95 b0 13|13 b0 95 7e) 	mulhdu\. r20,r21,r22
 5d0:	(7e 95 b0 12|12 b0 95 7e) 	mulhdu  r20,r21,r22
 5d4:	(7e 95 b0 97|97 b0 95 7e) 	mulhw\.  r20,r21,r22
 5d8:	(7e 95 b0 96|96 b0 95 7e) 	mulhw   r20,r21,r22
 5dc:	(7e 95 b0 17|17 b0 95 7e) 	mulhwu\. r20,r21,r22
 5e0:	(7e 95 b0 16|16 b0 95 7e) 	mulhwu  r20,r21,r22
 5e4:	(7e 95 b1 d3|d3 b1 95 7e) 	mulld\.  r20,r21,r22
 5e8:	(7e 95 b1 d2|d2 b1 95 7e) 	mulld   r20,r21,r22
 5ec:	(7e 95 b5 d3|d3 b5 95 7e) 	mulldo\. r20,r21,r22
 5f0:	(7e 95 b5 d2|d2 b5 95 7e) 	mulldo  r20,r21,r22
 5f4:	(1e 95 00 64|64 00 95 1e) 	mulli   r20,r21,100
 5f8:	(1e 95 ff 9c|9c ff 95 1e) 	mulli   r20,r21,-100
 5fc:	(7e 95 b1 d7|d7 b1 95 7e) 	mullw\.  r20,r21,r22
 600:	(7e 95 b1 d6|d6 b1 95 7e) 	mullw   r20,r21,r22
 604:	(7e 95 b5 d7|d7 b5 95 7e) 	mullwo\. r20,r21,r22
 608:	(7e 95 b5 d6|d6 b5 95 7e) 	mullwo  r20,r21,r22
 60c:	(7e b4 b3 b9|b9 b3 b4 7e) 	nand\.   r20,r21,r22
 610:	(7e b4 b3 b8|b8 b3 b4 7e) 	nand    r20,r21,r22
 614:	(7e 95 00 d1|d1 00 95 7e) 	neg\.    r20,r21
 618:	(7e 95 00 d0|d0 00 95 7e) 	neg     r20,r21
 61c:	(7e 95 04 d1|d1 04 95 7e) 	nego\.   r20,r21
 620:	(7e 95 04 d0|d0 04 95 7e) 	nego    r20,r21
 624:	(7e b4 b0 f9|f9 b0 b4 7e) 	nor\.    r20,r21,r22
 628:	(7e b4 b0 f8|f8 b0 b4 7e) 	nor     r20,r21,r22
 62c:	(7e b4 b3 79|79 b3 b4 7e) 	or\.     r20,r21,r22
 630:	(7e b4 b3 78|78 b3 b4 7e) 	or      r20,r21,r22
 634:	(7e b4 b3 39|39 b3 b4 7e) 	orc\.    r20,r21,r22
 638:	(7e b4 b3 38|38 b3 b4 7e) 	orc     r20,r21,r22
 63c:	(62 b4 10 00|00 10 b4 62) 	ori     r20,r21,4096
 640:	(66 b4 10 00|00 10 b4 66) 	oris    r20,r21,4096
 644:	(7d 6a 00 f4|f4 00 6a 7d) 	popcntb r10,r11
 648:	(7d 6a 03 f4|f4 03 6a 7d) 	popcntd r10,r11
 64c:	(7d 6a 02 f4|f4 02 6a 7d) 	popcntw r10,r11
 650:	(7d 6a 01 74|74 01 6a 7d) 	prtyd   r10,r11
 654:	(7d 6a 01 34|34 01 6a 7d) 	prtyw   r10,r11
 658:	(4c 00 00 66|66 00 00 4c) 	rfci
 65c:	(4c 00 00 cc|cc 00 00 4c) 	rfgi
 660:	(4c 00 00 64|64 00 00 4c) 	rfi
 664:	(4c 00 00 4c|4c 00 00 4c) 	rfmci
 668:	(79 6a 67 f1|f1 67 6a 79) 	rldcl\.  r10,r11,r12,63
 66c:	(79 6a 67 f0|f0 67 6a 79) 	rldcl   r10,r11,r12,63
 670:	(79 6a 67 f3|f3 67 6a 79) 	rldcr\.  r10,r11,r12,63
 674:	(79 6a 67 f2|f2 67 6a 79) 	rldcr   r10,r11,r12,63
 678:	(79 6a bf e9|e9 bf 6a 79) 	rldic\.  r10,r11,23,63
 67c:	(79 6a bf e8|e8 bf 6a 79) 	rldic   r10,r11,23,63
 680:	(79 6a bf e1|e1 bf 6a 79) 	rldicl\. r10,r11,23,63
 684:	(79 6a bf e0|e0 bf 6a 79) 	rldicl  r10,r11,23,63
 688:	(79 6a bf e5|e5 bf 6a 79) 	rldicr\. r10,r11,23,63
 68c:	(79 6a bf e4|e4 bf 6a 79) 	rldicr  r10,r11,23,63
 690:	(79 6a bf ed|ed bf 6a 79) 	rldimi\. r10,r11,23,63
 694:	(79 6a bf ec|ec bf 6a 79) 	rldimi  r10,r11,23,63
 698:	(51 6a b8 3f|3f b8 6a 51) 	rlwimi\. r10,r11,23,0,31
 69c:	(51 6a b8 3e|3e b8 6a 51) 	rlwimi  r10,r11,23,0,31
 6a0:	(55 6a b8 3f|3f b8 6a 55) 	rotlwi\. r10,r11,23
 6a4:	(55 6a b8 3e|3e b8 6a 55) 	rotlwi  r10,r11,23
 6a8:	(5d 6a b8 3f|3f b8 6a 5d) 	rotlw\.  r10,r11,r23
 6ac:	(5d 6a b8 3e|3e b8 6a 5d) 	rotlw   r10,r11,r23
 6b0:	(44 00 00 02|02 00 00 44) 	sc      
 6b4:	(44 00 0c 82|82 0c 00 44) 	sc      100
 6b8:	(7d 6a 60 37|37 60 6a 7d) 	sld\.    r10,r11,r12
 6bc:	(7d 6a 60 36|36 60 6a 7d) 	sld     r10,r11,r12
 6c0:	(7d 6a 60 31|31 60 6a 7d) 	slw\.    r10,r11,r12
 6c4:	(7d 6a 60 30|30 60 6a 7d) 	slw     r10,r11,r12
 6c8:	(7d 6a 66 35|35 66 6a 7d) 	srad\.   r10,r11,r12
 6cc:	(7d 6a 66 34|34 66 6a 7d) 	srad    r10,r11,r12
 6d0:	(7d 6a fe 77|77 fe 6a 7d) 	sradi\.  r10,r11,63
 6d4:	(7d 6a fe 76|76 fe 6a 7d) 	sradi   r10,r11,63
 6d8:	(7d 6a 66 31|31 66 6a 7d) 	sraw\.   r10,r11,r12
 6dc:	(7d 6a 66 30|30 66 6a 7d) 	sraw    r10,r11,r12
 6e0:	(7d 6a fe 71|71 fe 6a 7d) 	srawi\.  r10,r11,31
 6e4:	(7d 6a fe 70|70 fe 6a 7d) 	srawi   r10,r11,31
 6e8:	(7d 6a 64 37|37 64 6a 7d) 	srd\.    r10,r11,r12
 6ec:	(7d 6a 64 36|36 64 6a 7d) 	srd     r10,r11,r12
 6f0:	(7d 6a 64 31|31 64 6a 7d) 	srw\.    r10,r11,r12
 6f4:	(7d 6a 64 30|30 64 6a 7d) 	srw     r10,r11,r12
 6f8:	(99 4b ff ff|ff ff 4b 99) 	stb     r10,-1\(r11\)
 6fc:	(99 4b 00 01|01 00 4b 99) 	stb     r10,1\(r11\)
 700:	(7d 4b 61 be|be 61 4b 7d) 	stbepx  r10,r11,r12
 704:	(9d 4b ff ff|ff ff 4b 9d) 	stbu    r10,-1\(r11\)
 708:	(9d 4b 00 01|01 00 4b 9d) 	stbu    r10,1\(r11\)
 70c:	(7d 4b 61 ee|ee 61 4b 7d) 	stbux   r10,r11,r12
 710:	(7d 4b 61 ae|ae 61 4b 7d) 	stbx    r10,r11,r12
 714:	(f9 4b ff f8|f8 ff 4b f9) 	std     r10,-8\(r11\)
 718:	(f9 4b 00 08|08 00 4b f9) 	std     r10,8\(r11\)
 71c:	(7d 4b 65 28|28 65 4b 7d) 	stdbrx  r10,r11,r12
 720:	(7d 4b 61 ad|ad 61 4b 7d) 	stdcx\.  r10,r11,r12
 724:	(7d 4b 61 3a|3a 61 4b 7d) 	stdepx  r10,r11,r12
 728:	(f9 4b ff f9|f9 ff 4b f9) 	stdu    r10,-8\(r11\)
 72c:	(f9 4b 00 09|09 00 4b f9) 	stdu    r10,8\(r11\)
 730:	(7d 4b 61 6a|6a 61 4b 7d) 	stdux   r10,r11,r12
 734:	(7d 4b 61 2a|2a 61 4b 7d) 	stdx    r10,r11,r12
 738:	(da 8a ff f8|f8 ff 8a da) 	stfd    f20,-8\(r10\)
 73c:	(da 8a 00 08|08 00 8a da) 	stfd    f20,8\(r10\)
 740:	(7e 8a 5d be|be 5d 8a 7e) 	stfdepx f20,r10,r11
 744:	(de 8a ff f8|f8 ff 8a de) 	stfdu   f20,-8\(r10\)
 748:	(de 8a 00 08|08 00 8a de) 	stfdu   f20,8\(r10\)
 74c:	(7e 8a 5d ee|ee 5d 8a 7e) 	stfdux  f20,r10,r11
 750:	(7e 8a 5d ae|ae 5d 8a 7e) 	stfdx   f20,r10,r11
 754:	(7e 8a 5f ae|ae 5f 8a 7e) 	stfiwx  f20,r10,r11
 758:	(d2 8a ff fc|fc ff 8a d2) 	stfs    f20,-4\(r10\)
 75c:	(d2 8a 00 04|04 00 8a d2) 	stfs    f20,4\(r10\)
 760:	(d6 8a ff fc|fc ff 8a d6) 	stfsu   f20,-4\(r10\)
 764:	(d6 8a 00 04|04 00 8a d6) 	stfsu   f20,4\(r10\)
 768:	(7e 8a 5d 6e|6e 5d 8a 7e) 	stfsux  f20,r10,r11
 76c:	(7e 8a 5d 2e|2e 5d 8a 7e) 	stfsx   f20,r10,r11
 770:	(b1 4b ff fe|fe ff 4b b1) 	sth     r10,-2\(r11\)
 774:	(b1 4b 00 02|02 00 4b b1) 	sth     r10,2\(r11\)
 778:	(b1 4b ff fc|fc ff 4b b1) 	sth     r10,-4\(r11\)
 77c:	(b1 4b 00 04|04 00 4b b1) 	sth     r10,4\(r11\)
 780:	(7d 4b 67 2c|2c 67 4b 7d) 	sthbrx  r10,r11,r12
 784:	(7d 4b 63 3e|3e 63 4b 7d) 	sthepx  r10,r11,r12
 788:	(b5 4b ff fe|fe ff 4b b5) 	sthu    r10,-2\(r11\)
 78c:	(b5 4b 00 02|02 00 4b b5) 	sthu    r10,2\(r11\)
 790:	(7d 4b 63 6e|6e 63 4b 7d) 	sthux   r10,r11,r12
 794:	(7d 4b 63 2e|2e 63 4b 7d) 	sthx    r10,r11,r12
 798:	(be 8a 00 10|10 00 8a be) 	stmw    r20,16\(r10\)
 79c:	(7d 4b 0d aa|aa 0d 4b 7d) 	stswi   r10,r11,1
 7a0:	(7d 4b 05 aa|aa 05 4b 7d) 	stswi   r10,r11,32
 7a4:	(7d 4b 65 2a|2a 65 4b 7d) 	stswx   r10,r11,r12
 7a8:	(7d 4b 65 2c|2c 65 4b 7d) 	stwbrx  r10,r11,r12
 7ac:	(7d 4b 61 2d|2d 61 4b 7d) 	stwcx\.  r10,r11,r12
 7b0:	(7d 4b 61 3e|3e 61 4b 7d) 	stwepx  r10,r11,r12
 7b4:	(95 4b ff fc|fc ff 4b 95) 	stwu    r10,-4\(r11\)
 7b8:	(95 4b 00 04|04 00 4b 95) 	stwu    r10,4\(r11\)
 7bc:	(7d 4b 61 6e|6e 61 4b 7d) 	stwux   r10,r11,r12
 7c0:	(7d 4b 61 2e|2e 61 4b 7d) 	stwx    r10,r11,r12
 7c4:	(7e 95 b0 51|51 b0 95 7e) 	subf\.   r20,r21,r22
 7c8:	(7e 95 b0 50|50 b0 95 7e) 	subf    r20,r21,r22
 7cc:	(7e 95 b0 11|11 b0 95 7e) 	subfc\.  r20,r21,r22
 7d0:	(7e 95 b0 10|10 b0 95 7e) 	subfc   r20,r21,r22
 7d4:	(7e 95 b4 11|11 b4 95 7e) 	subfco\. r20,r21,r22
 7d8:	(7e 95 b4 10|10 b4 95 7e) 	subfco  r20,r21,r22
 7dc:	(7e 95 b1 11|11 b1 95 7e) 	subfe\.  r20,r21,r22
 7e0:	(7e 95 b1 10|10 b1 95 7e) 	subfe   r20,r21,r22
 7e4:	(7e 95 b5 11|11 b5 95 7e) 	subfeo\. r20,r21,r22
 7e8:	(7e 95 b5 10|10 b5 95 7e) 	subfeo  r20,r21,r22
 7ec:	(22 95 00 64|64 00 95 22) 	subfic  r20,r21,100
 7f0:	(22 95 ff 9c|9c ff 95 22) 	subfic  r20,r21,-100
 7f4:	(7e 95 01 d1|d1 01 95 7e) 	subfme\. r20,r21
 7f8:	(7e 95 01 d0|d0 01 95 7e) 	subfme  r20,r21
 7fc:	(7e 95 05 d1|d1 05 95 7e) 	subfmeo\. r20,r21
 800:	(7e 95 05 d0|d0 05 95 7e) 	subfmeo r20,r21
 804:	(7e 95 b4 51|51 b4 95 7e) 	subfo\.  r20,r21,r22
 808:	(7e 95 b4 50|50 b4 95 7e) 	subfo   r20,r21,r22
 80c:	(7e 95 01 91|91 01 95 7e) 	subfze\. r20,r21
 810:	(7e 95 01 90|90 01 95 7e) 	subfze  r20,r21
 814:	(7e 95 05 91|91 05 95 7e) 	subfzeo\. r20,r21
 818:	(7e 95 05 90|90 05 95 7e) 	subfzeo r20,r21
 81c:	(7c 00 04 ac|ac 04 00 7c) 	sync    
 820:	(7c 00 04 ac|ac 04 00 7c) 	sync    
 824:	(7c 00 04 ac|ac 04 00 7c) 	sync    
 828:	(7c 20 04 ac|ac 04 20 7c) 	lwsync
 82c:	(7c aa 58 88|88 58 aa 7c) 	tdlge   r10,r11
 830:	(08 aa 00 64|64 00 aa 08) 	tdlgei  r10,100
 834:	(08 aa ff 9c|9c ff aa 08) 	tdlgei  r10,-100
 838:	(7c 6a 58 24|24 58 6a 7c) 	tlbilxva r10,r11
 83c:	(7c 0a 5e 24|24 5e 0a 7c) 	tlbivax r10,r11
 840:	(7c 00 07 64|64 07 00 7c) 	tlbre   
 844:	(7d 4b 3f 64|64 3f 4b 7d) 	tlbre   r10,r11,7
 848:	(7c 0a 5e a5|a5 5e 0a 7c) 	tlbsrx\. r10,r11
 84c:	(7d 4b 67 25|25 67 4b 7d) 	tlbsx\.  r10,r11,r12
 850:	(7d 4b 67 24|24 67 4b 7d) 	tlbsx   r10,r11,r12
 854:	(7c 00 04 6c|6c 04 00 7c) 	tlbsync
 858:	(7c 00 07 a4|a4 07 00 7c) 	tlbwe   
 85c:	(7d 4b 3f a4|a4 3f 4b 7d) 	tlbwe   r10,r11,7
 860:	(7c aa 58 08|08 58 aa 7c) 	twlge   r10,r11
 864:	(0c aa 00 64|64 00 aa 0c) 	twlgei  r10,100
 868:	(0c aa ff 9c|9c ff aa 0c) 	twlgei  r10,-100
 86c:	(7c 00 00 7c|7c 00 00 7c) 	wait    
 870:	(7c 00 00 7c|7c 00 00 7c) 	wait    
 874:	(7c 20 00 7c|7c 00 20 7c) 	waitrsv
 878:	(7c 40 00 7c|7c 00 40 7c) 	waitimpl
 87c:	(7c 40 00 7c|7c 00 40 7c) 	waitimpl
 880:	(7c 20 00 7c|7c 00 20 7c) 	waitrsv
 884:	(7c 00 01 6c|6c 01 00 7c) 	wchkall 
 888:	(7c 00 01 6c|6c 01 00 7c) 	wchkall 
 88c:	(7d 80 01 6c|6c 01 80 7d) 	wchkall cr3
 890:	(7c 2a 5f 4c|4c 5f 2a 7c) 	wclr    1,r10,r11
 894:	(7c 20 07 4c|4c 07 20 7c) 	wclrall 1
 898:	(7c 4a 5f 4c|4c 5f 4a 7c) 	wclrone r10,r11
 89c:	(7d 40 01 06|06 01 40 7d) 	wrtee   r10
 8a0:	(7c 00 81 46|46 81 00 7c) 	wrteei  1
 8a4:	(7d 6a 62 79|79 62 6a 7d) 	xor\.    r10,r11,r12
 8a8:	(7d 6a 62 78|78 62 6a 7d) 	xor     r10,r11,r12
 8ac:	(69 6a 10 00|00 10 6a 69) 	xori    r10,r11,4096
 8b0:	(6d 6a 10 00|00 10 6a 6d) 	xoris   r10,r11,4096
