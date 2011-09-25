#as: -mpower7
#objdump: -dr -Mpower7
#name: POWER7 tests (includes DFP, Altivec and VSX)

.*: +file format elf(32)?(64)?-powerpc.*

Disassembly of section \.text:

0+00 <power7>:
   0:	7c 64 2e 98 	lxvd2x  vs3,r4,r5
   4:	7d 64 2e 99 	lxvd2x  vs43,r4,r5
   8:	7c 64 2f 98 	stxvd2x vs3,r4,r5
   c:	7d 64 2f 99 	stxvd2x vs43,r4,r5
  10:	f0 64 28 50 	xxmrghd vs3,vs4,vs5
  14:	f1 6c 68 57 	xxmrghd vs43,vs44,vs45
  18:	f0 64 2b 50 	xxmrgld vs3,vs4,vs5
  1c:	f1 6c 6b 57 	xxmrgld vs43,vs44,vs45
  20:	f0 64 28 50 	xxmrghd vs3,vs4,vs5
  24:	f1 6c 68 57 	xxmrghd vs43,vs44,vs45
  28:	f0 64 2b 50 	xxmrgld vs3,vs4,vs5
  2c:	f1 6c 6b 57 	xxmrgld vs43,vs44,vs45
  30:	f0 64 29 50 	xxpermdi vs3,vs4,vs5,1
  34:	f1 6c 69 57 	xxpermdi vs43,vs44,vs45,1
  38:	f0 64 2a 50 	xxpermdi vs3,vs4,vs5,2
  3c:	f1 6c 6a 57 	xxpermdi vs43,vs44,vs45,2
  40:	f0 64 27 80 	xvmovdp vs3,vs4
  44:	f1 6c 67 87 	xvmovdp vs43,vs44
  48:	f0 64 27 80 	xvmovdp vs3,vs4
  4c:	f1 6c 67 87 	xvmovdp vs43,vs44
  50:	f0 64 2f 80 	xvcpsgndp vs3,vs4,vs5
  54:	f1 6c 6f 87 	xvcpsgndp vs43,vs44,vs45
  58:	7c 00 00 7c 	wait    
  5c:	7c 00 00 7c 	wait    
  60:	7c 20 00 7c 	waitrsv
  64:	7c 20 00 7c 	waitrsv
  68:	7c 40 00 7c 	waitimpl
  6c:	7c 40 00 7c 	waitimpl
  70:	4c 00 03 24 	doze
  74:	4c 00 03 64 	nap
  78:	4c 00 03 a4 	sleep
  7c:	4c 00 03 e4 	rvwinkle
  80:	7c 83 01 34 	prtyw   r3,r4
  84:	7d cd 01 74 	prtyd   r13,r14
  88:	7d 5c 02 a6 	mfcfar  r10
  8c:	7d 7c 03 a6 	mtcfar  r11
  90:	7c 83 2b f8 	cmpb    r3,r4,r5
  94:	7d 4b 66 2a 	lwzcix  r10,r11,r12
  98:	ee 11 90 04 	dadd    f16,f17,f18
  9c:	fe 96 c0 04 	daddq   f20,f22,f24
  a0:	7c 60 06 6c 	dss     3
  a4:	7e 00 06 6c 	dssall
  a8:	7c 25 22 ac 	dst     r5,r4,1
  ac:	7e 08 3a ac 	dstt    r8,r7,0
  b0:	7c 65 32 ec 	dstst   r5,r6,3
  b4:	7e 44 2a ec 	dststt  r4,r5,2
  b8:	7d 4b 63 56 	divwe   r10,r11,r12
  bc:	7d 6c 6b 57 	divwe\.  r11,r12,r13
  c0:	7d 8d 77 56 	divweo  r12,r13,r14
  c4:	7d ae 7f 57 	divweo\. r13,r14,r15
  c8:	7d 4b 63 16 	divweu  r10,r11,r12
  cc:	7d 6c 6b 17 	divweu\. r11,r12,r13
  d0:	7d 8d 77 16 	divweuo r12,r13,r14
  d4:	7d ae 7f 17 	divweuo\. r13,r14,r15
  d8:	7e 27 d9 f8 	bpermd  r7,r17,r27
  dc:	7e 8a 02 f4 	popcntw r10,r20
  e0:	7e 8a 03 f4 	popcntd r10,r20
  e4:	7e 95 b4 28 	ldbrx   r20,r21,r22
  e8:	7e 95 b5 28 	stdbrx  r20,r21,r22
  ec:	7d 40 56 ee 	lfiwzx  f10,0,r10
  f0:	7d 49 56 ee 	lfiwzx  f10,r9,r10
  f4:	ec 80 2e 9c 	fcfids  f4,f5
  f8:	ec 80 2e 9d 	fcfids\. f4,f5
  fc:	ec 80 2f 9c 	fcfidus f4,f5
 100:	ec 80 2f 9d 	fcfidus\. f4,f5
 104:	fc 80 29 1c 	fctiwu  f4,f5
 108:	fc 80 29 1d 	fctiwu\. f4,f5
 10c:	fc 80 29 1e 	fctiwuz f4,f5
 110:	fc 80 29 1f 	fctiwuz\. f4,f5
 114:	fc 80 2f 5c 	fctidu  f4,f5
 118:	fc 80 2f 5d 	fctidu\. f4,f5
 11c:	fc 80 2f 5e 	fctiduz f4,f5
 120:	fc 80 2f 5f 	fctiduz\. f4,f5
 124:	fc 80 2f 9c 	fcfidu  f4,f5
 128:	fc 80 2f 9d 	fcfidu\. f4,f5
 12c:	fc 0a 59 00 	ftdiv   cr0,f10,f11
 130:	ff 8a 59 00 	ftdiv   cr7,f10,f11
 134:	fc 00 51 40 	ftsqrt  cr0,f10
 138:	ff 80 51 40 	ftsqrt  cr7,f10
 13c:	7e 08 4a 2c 	dcbtt   r8,r9
 140:	7e 08 49 ec 	dcbtstt r8,r9
 144:	ed 40 66 44 	dcffix  f10,f12
 148:	ee 80 b6 45 	dcffix\. f20,f22
 14c:	7d 4b 60 68 	lbarx   r10,r11,r12
 150:	7d 4b 60 68 	lbarx   r10,r11,r12
 154:	7d 4b 60 69 	lbarx   r10,r11,r12,1
 158:	7e 95 b0 e8 	lharx   r20,r21,r22
 15c:	7e 95 b0 e8 	lharx   r20,r21,r22
 160:	7e 95 b0 e9 	lharx   r20,r21,r22,1
 164:	7d 4b 65 6d 	stbcx\.  r10,r11,r12
 168:	7d 4b 65 ad 	sthcx\.  r10,r11,r12
 16c:	fd c0 78 30 	fre     f14,f15
 170:	fd c0 78 31 	fre\.    f14,f15
 174:	ed c0 78 30 	fres    f14,f15
 178:	ed c0 78 31 	fres\.   f14,f15
 17c:	fd c0 78 34 	frsqrte f14,f15
 180:	fd c0 78 35 	frsqrte\. f14,f15
 184:	ed c0 78 34 	frsqrtes f14,f15
 188:	ed c0 78 35 	frsqrtes\. f14,f15
 18c:	7c 43 27 1e 	isel    r2,r3,r4,28
 190:	60 42 00 00 	ori     r2,r2,0
 194:	60 00 00 00 	nop
 198:	60 00 00 00 	nop
 19c:	60 42 00 00 	ori     r2,r2,0
