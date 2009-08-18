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

