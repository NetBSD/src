#as: -mppc -me500mc
#objdump: -dr -Me500mc
#name: Power E500MC tests

.*: +file format elf(32)?(64)?-powerpc.*

Disassembly of section \.text:

0+0000000 <start>:
   0:	4c 00 00 4e 	rfdi
   4:	4c 00 00 cc 	rfgi
   8:	4c 1f f9 8c 	dnh     0,1023
   c:	4f e0 01 8c 	dnh     31,0
  10:	7c 09 57 be 	icbiep  r9,r10
  14:	7c 00 69 dc 	msgclr  r13
  18:	7c 00 71 9c 	msgsnd  r14
  1c:	7c 00 00 7c 	wait
  20:	7f 9c e3 78 	mdors
  24:	7c 00 02 1c 	ehpriv
  28:	7c 18 cb c6 	dsn     r24,r25
  2c:	7c 22 18 be 	lbepx   r1,r2,r3
  30:	7c 85 32 3e 	lhepx   r4,r5,r6
  34:	7c e8 48 3e 	lwepx   r7,r8,r9
  38:	7d 4b 60 3a 	ldepx   r10,r11,r12
  3c:	7d ae 7c be 	lfdepx  r13,r14,r15
  40:	7e 11 91 be 	stbepx  r16,r17,r18
  44:	7e 74 ab 3e 	sthepx  r19,r20,r21
  48:	7e d7 c1 3e 	stwepx  r22,r23,r24
  4c:	7f 3a d9 3a 	stdepx  r25,r26,r27
  50:	7f 9d f5 be 	stfdepx r28,r29,r30
  54:	7c 01 14 06 	lbdx    r0,r1,r2
  58:	7d 8d 74 46 	lhdx    r12,r13,r14
  5c:	7c 64 2c 86 	lwdx    r3,r4,r5
  60:	7f 5b e6 46 	lfddx   f26,r27,r28
  64:	7d f0 8c c6 	lddx    r15,r16,r17
  68:	7c c7 45 06 	stbdx   r6,r7,r8
  6c:	7e 53 a5 46 	sthdx   r18,r19,r20
  70:	7d 2a 5d 86 	stwdx   r9,r10,r11
  74:	7f be ff 46 	stfddx  f29,r30,r31
  78:	7e b6 bd c6 	stddx   r21,r22,r23
  7c:	7c 20 0d ec 	dcbal   r0,r1
  80:	7c 26 3f ec 	dcbzl   r6,r7
  84:	7c 1f 00 7e 	dcbstep r31,r0
  88:	7c 01 10 fe 	dcbfep  r1,r2
  8c:	7c 64 29 fe 	dcbtstep r3,r4,r5
  90:	7c c7 42 7e 	dcbtep  r6,r7,r8
  94:	7c 0b 67 fe 	dcbzep  r11,r12
  98:	7c 00 06 26 	tlbilx  0,0,r0
  9c:	7c 20 06 26 	tlbilx  1,0,r0
  a0:	7c 62 1e 26 	tlbilx  3,r2,r3
  a4:	7c 64 2e 26 	tlbilx  3,r4,r5
