#as: -a32 -mpower7
#objdump: -dr -Mpower7
#name: POWER7 tests (includes DFP, Altivec and VSX)

.*: +file format elf32-powerpc.*

Disassembly of section \.text:

0+00 <power7>:
   0:	7c 64 2e 98 	lxvd2x  vs3,r4,r5
   4:	7c 64 2e d8 	lxvd2ux vs3,r4,r5
   8:	7d 64 2e 99 	lxvd2x  vs43,r4,r5
   c:	7d 64 2e d9 	lxvd2ux vs43,r4,r5
  10:	7c 64 2f 98 	stxvd2x vs3,r4,r5
  14:	7c 64 2f d8 	stxvd2ux vs3,r4,r5
  18:	7d 64 2f 99 	stxvd2x vs43,r4,r5
  1c:	7d 64 2f d9 	stxvd2ux vs43,r4,r5
  20:	f0 64 28 50 	xxmrghd vs3,vs4,vs5
  24:	f1 6c 68 57 	xxmrghd vs43,vs44,vs45
  28:	f0 64 2b 50 	xxmrgld vs3,vs4,vs5
  2c:	f1 6c 6b 57 	xxmrgld vs43,vs44,vs45
  30:	f0 64 28 50 	xxmrghd vs3,vs4,vs5
  34:	f1 6c 68 57 	xxmrghd vs43,vs44,vs45
  38:	f0 64 2b 50 	xxmrgld vs3,vs4,vs5
  3c:	f1 6c 6b 57 	xxmrgld vs43,vs44,vs45
  40:	f0 64 29 50 	xxpermdi vs3,vs4,vs5,1
  44:	f1 6c 69 57 	xxpermdi vs43,vs44,vs45,1
  48:	f0 64 2a 50 	xxpermdi vs3,vs4,vs5,2
  4c:	f1 6c 6a 57 	xxpermdi vs43,vs44,vs45,2
  50:	f0 64 27 80 	xvmovdp vs3,vs4
  54:	f1 6c 67 87 	xvmovdp vs43,vs44
  58:	f0 64 27 80 	xvmovdp vs3,vs4
  5c:	f1 6c 67 87 	xvmovdp vs43,vs44
  60:	f0 64 2f 80 	xvcpsgndp vs3,vs4,vs5
  64:	f1 6c 6f 87 	xvcpsgndp vs43,vs44,vs45
  68:	4c 00 03 24 	doze
  6c:	4c 00 03 64 	nap
  70:	4c 00 03 a4 	sleep
  74:	4c 00 03 e4 	rvwinkle
  78:	7c 83 01 34 	prtyw   r3,r4
  7c:	7d cd 01 74 	prtyd   r13,r14
  80:	7d 5c 02 a6 	mfcfar  r10
  84:	7d 7c 03 a6 	mtcfar  r11
  88:	7c 83 2b f8 	cmpb    r3,r4,r5
  8c:	7c c0 3c be 	mffgpr  f6,r7
  90:	7d 00 4d be 	mftgpr  r8,f9
  94:	7d 4b 66 2a 	lwzcix  r10,r11,r12
  98:	7d ae 7e 2e 	lfdpx   f13,r14,r15
  9c:	ee 11 90 04 	dadd    f16,f17,f18
  a0:	fe 96 c0 04 	daddq   f20,f22,f24
  a4:	7c 60 06 6c 	dss     3
  a8:	7e 00 06 6c 	dssall
  ac:	7c 25 22 ac 	dst     r5,r4,1
  b0:	7e 08 3a ac 	dstt    r8,r7,0
  b4:	7c 65 32 ec 	dstst   r5,r6,3
  b8:	7e 44 2a ec 	dststt  r4,r5,2
  bc:	4e 80 00 20 	blr
