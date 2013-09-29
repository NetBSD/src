#as: -mpower7
#objdump: -dr -Mpower7
#name: POWER7 tests (includes DFP, Altivec and VSX)

.*

Disassembly of section \.text:

0+00 <power7>:
   0:	(7c 64 2e 98|98 2e 64 7c) 	lxvd2x  vs3,r4,r5
   4:	(7d 64 2e 99|99 2e 64 7d) 	lxvd2x  vs43,r4,r5
   8:	(7c 64 2f 98|98 2f 64 7c) 	stxvd2x vs3,r4,r5
   c:	(7d 64 2f 99|99 2f 64 7d) 	stxvd2x vs43,r4,r5
  10:	(f0 64 28 50|50 28 64 f0) 	xxmrghd vs3,vs4,vs5
  14:	(f1 6c 68 57|57 68 6c f1) 	xxmrghd vs43,vs44,vs45
  18:	(f0 64 2b 50|50 2b 64 f0) 	xxmrgld vs3,vs4,vs5
  1c:	(f1 6c 6b 57|57 6b 6c f1) 	xxmrgld vs43,vs44,vs45
  20:	(f0 64 28 50|50 28 64 f0) 	xxmrghd vs3,vs4,vs5
  24:	(f1 6c 68 57|57 68 6c f1) 	xxmrghd vs43,vs44,vs45
  28:	(f0 64 2b 50|50 2b 64 f0) 	xxmrgld vs3,vs4,vs5
  2c:	(f1 6c 6b 57|57 6b 6c f1) 	xxmrgld vs43,vs44,vs45
  30:	(f0 64 29 50|50 29 64 f0) 	xxpermdi vs3,vs4,vs5,1
  34:	(f1 6c 69 57|57 69 6c f1) 	xxpermdi vs43,vs44,vs45,1
  38:	(f0 64 2a 50|50 2a 64 f0) 	xxpermdi vs3,vs4,vs5,2
  3c:	(f1 6c 6a 57|57 6a 6c f1) 	xxpermdi vs43,vs44,vs45,2
  40:	(f0 64 27 80|80 27 64 f0) 	xvmovdp vs3,vs4
  44:	(f1 6c 67 87|87 67 6c f1) 	xvmovdp vs43,vs44
  48:	(f0 64 27 80|80 27 64 f0) 	xvmovdp vs3,vs4
  4c:	(f1 6c 67 87|87 67 6c f1) 	xvmovdp vs43,vs44
  50:	(f0 64 2f 80|80 2f 64 f0) 	xvcpsgndp vs3,vs4,vs5
  54:	(f1 6c 6f 87|87 6f 6c f1) 	xvcpsgndp vs43,vs44,vs45
  58:	(7c 00 00 7c|7c 00 00 7c) 	wait    
  5c:	(7c 00 00 7c|7c 00 00 7c) 	wait    
  60:	(7c 20 00 7c|7c 00 20 7c) 	waitrsv
  64:	(7c 20 00 7c|7c 00 20 7c) 	waitrsv
  68:	(7c 40 00 7c|7c 00 40 7c) 	waitimpl
  6c:	(7c 40 00 7c|7c 00 40 7c) 	waitimpl
  70:	(4c 00 03 24|24 03 00 4c) 	doze
  74:	(4c 00 03 64|64 03 00 4c) 	nap
  78:	(4c 00 03 a4|a4 03 00 4c) 	sleep
  7c:	(4c 00 03 e4|e4 03 00 4c) 	rvwinkle
  80:	(7c 83 01 34|34 01 83 7c) 	prtyw   r3,r4
  84:	(7d cd 01 74|74 01 cd 7d) 	prtyd   r13,r14
  88:	(7d 5c 02 a6|a6 02 5c 7d) 	mfcfar  r10
  8c:	(7d 7c 03 a6|a6 03 7c 7d) 	mtcfar  r11
  90:	(7c 83 2b f8|f8 2b 83 7c) 	cmpb    r3,r4,r5
  94:	(7d 4b 66 2a|2a 66 4b 7d) 	lwzcix  r10,r11,r12
  98:	(ee 11 90 04|04 90 11 ee) 	dadd    f16,f17,f18
  9c:	(fe 96 c0 04|04 c0 96 fe) 	daddq   f20,f22,f24
  a0:	(7c 60 06 6c|6c 06 60 7c) 	dss     3
  a4:	(7e 00 06 6c|6c 06 00 7e) 	dssall
  a8:	(7c 25 22 ac|ac 22 25 7c) 	dst     r5,r4,1
  ac:	(7e 08 3a ac|ac 3a 08 7e) 	dstt    r8,r7,0
  b0:	(7c 65 32 ec|ec 32 65 7c) 	dstst   r5,r6,3
  b4:	(7e 44 2a ec|ec 2a 44 7e) 	dststt  r4,r5,2
  b8:	(7d 4b 63 56|56 63 4b 7d) 	divwe   r10,r11,r12
  bc:	(7d 6c 6b 57|57 6b 6c 7d) 	divwe\.  r11,r12,r13
  c0:	(7d 8d 77 56|56 77 8d 7d) 	divweo  r12,r13,r14
  c4:	(7d ae 7f 57|57 7f ae 7d) 	divweo\. r13,r14,r15
  c8:	(7d 4b 63 16|16 63 4b 7d) 	divweu  r10,r11,r12
  cc:	(7d 6c 6b 17|17 6b 6c 7d) 	divweu\. r11,r12,r13
  d0:	(7d 8d 77 16|16 77 8d 7d) 	divweuo r12,r13,r14
  d4:	(7d ae 7f 17|17 7f ae 7d) 	divweuo\. r13,r14,r15
  d8:	(7e 27 d9 f8|f8 d9 27 7e) 	bpermd  r7,r17,r27
  dc:	(7e 8a 02 f4|f4 02 8a 7e) 	popcntw r10,r20
  e0:	(7e 8a 03 f4|f4 03 8a 7e) 	popcntd r10,r20
  e4:	(7e 95 b4 28|28 b4 95 7e) 	ldbrx   r20,r21,r22
  e8:	(7e 95 b5 28|28 b5 95 7e) 	stdbrx  r20,r21,r22
  ec:	(7d 40 56 ee|ee 56 40 7d) 	lfiwzx  f10,0,r10
  f0:	(7d 49 56 ee|ee 56 49 7d) 	lfiwzx  f10,r9,r10
  f4:	(ec 80 2e 9c|9c 2e 80 ec) 	fcfids  f4,f5
  f8:	(ec 80 2e 9d|9d 2e 80 ec) 	fcfids\. f4,f5
  fc:	(ec 80 2f 9c|9c 2f 80 ec) 	fcfidus f4,f5
 100:	(ec 80 2f 9d|9d 2f 80 ec) 	fcfidus\. f4,f5
 104:	(fc 80 29 1c|1c 29 80 fc) 	fctiwu  f4,f5
 108:	(fc 80 29 1d|1d 29 80 fc) 	fctiwu\. f4,f5
 10c:	(fc 80 29 1e|1e 29 80 fc) 	fctiwuz f4,f5
 110:	(fc 80 29 1f|1f 29 80 fc) 	fctiwuz\. f4,f5
 114:	(fc 80 2f 5c|5c 2f 80 fc) 	fctidu  f4,f5
 118:	(fc 80 2f 5d|5d 2f 80 fc) 	fctidu\. f4,f5
 11c:	(fc 80 2f 5e|5e 2f 80 fc) 	fctiduz f4,f5
 120:	(fc 80 2f 5f|5f 2f 80 fc) 	fctiduz\. f4,f5
 124:	(fc 80 2f 9c|9c 2f 80 fc) 	fcfidu  f4,f5
 128:	(fc 80 2f 9d|9d 2f 80 fc) 	fcfidu\. f4,f5
 12c:	(fc 0a 59 00|00 59 0a fc) 	ftdiv   cr0,f10,f11
 130:	(ff 8a 59 00|00 59 8a ff) 	ftdiv   cr7,f10,f11
 134:	(fc 00 51 40|40 51 00 fc) 	ftsqrt  cr0,f10
 138:	(ff 80 51 40|40 51 80 ff) 	ftsqrt  cr7,f10
 13c:	(7e 08 4a 2c|2c 4a 08 7e) 	dcbtt   r8,r9
 140:	(7e 08 49 ec|ec 49 08 7e) 	dcbtstt r8,r9
 144:	(ed 40 66 44|44 66 40 ed) 	dcffix  f10,f12
 148:	(ee 80 b6 45|45 b6 80 ee) 	dcffix\. f20,f22
 14c:	(7d 4b 60 68|68 60 4b 7d) 	lbarx   r10,r11,r12
 150:	(7d 4b 60 68|68 60 4b 7d) 	lbarx   r10,r11,r12
 154:	(7d 4b 60 69|69 60 4b 7d) 	lbarx   r10,r11,r12,1
 158:	(7e 95 b0 e8|e8 b0 95 7e) 	lharx   r20,r21,r22
 15c:	(7e 95 b0 e8|e8 b0 95 7e) 	lharx   r20,r21,r22
 160:	(7e 95 b0 e9|e9 b0 95 7e) 	lharx   r20,r21,r22,1
 164:	(7d 4b 65 6d|6d 65 4b 7d) 	stbcx\.  r10,r11,r12
 168:	(7d 4b 65 ad|ad 65 4b 7d) 	sthcx\.  r10,r11,r12
 16c:	(fd c0 78 30|30 78 c0 fd) 	fre     f14,f15
 170:	(fd c0 78 31|31 78 c0 fd) 	fre\.    f14,f15
 174:	(ed c0 78 30|30 78 c0 ed) 	fres    f14,f15
 178:	(ed c0 78 31|31 78 c0 ed) 	fres\.   f14,f15
 17c:	(fd c0 78 34|34 78 c0 fd) 	frsqrte f14,f15
 180:	(fd c0 78 35|35 78 c0 fd) 	frsqrte\. f14,f15
 184:	(ed c0 78 34|34 78 c0 ed) 	frsqrtes f14,f15
 188:	(ed c0 78 35|35 78 c0 ed) 	frsqrtes\. f14,f15
 18c:	(7c 43 27 1e|1e 27 43 7c) 	isel    r2,r3,r4,28
 190:	(60 42 00 00|00 00 42 60) 	ori     r2,r2,0
 194:	(60 00 00 00|00 00 00 60) 	nop
 198:	(60 00 00 00|00 00 00 60) 	nop
 19c:	(60 42 00 00|00 00 42 60) 	ori     r2,r2,0
 1a0:	(7f 7b db 78|78 db 7b 7f) 	yield
 1a4:	(7f 7b db 78|78 db 7b 7f) 	yield
 1a8:	(7f bd eb 78|78 eb bd 7f) 	mdoio
 1ac:	(7f bd eb 78|78 eb bd 7f) 	mdoio
 1b0:	(7f de f3 78|78 f3 de 7f) 	mdoom
 1b4:	(7f de f3 78|78 f3 de 7f) 	mdoom
 1b8:	(7d 40 e2 a6|a6 e2 40 7d) 	mfppr   r10
 1bc:	(7d 62 e2 a6|a6 e2 62 7d) 	mfppr32 r11
 1c0:	(7d 80 e3 a6|a6 e3 80 7d) 	mtppr   r12
 1c4:	(7d a2 e3 a6|a6 e3 a2 7d) 	mtppr32 r13
#pass
