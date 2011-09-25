#as: -mppc -me500mc
#objdump: -dr -Me500mc
#name: Power E500MC tests

.*: +file format elf(32)?(64)?-powerpc.*

Disassembly of section \.text:

0+00 <start>:
   0:	4c 00 00 4e 	rfdi
   4:	4c 00 00 cc 	rfgi
   8:	4c 1f f9 8c 	dnh     0,1023
   c:	4f e0 01 8c 	dnh     31,0
  10:	7c 09 57 be 	icbiep  r9,r10
  14:	7c 00 69 dc 	msgclr  r13
  18:	7c 00 71 9c 	msgsnd  r14
  1c:	7c 00 00 7c 	wait    
  20:	7c 00 00 7c 	wait    
  24:	7c 20 00 7c 	waitrsv
  28:	7c 20 00 7c 	waitrsv
  2c:	7c 40 00 7c 	waitimpl
  30:	7c 40 00 7c 	waitimpl
  34:	7f 9c e3 78 	mdors
  38:	7c 00 02 1c 	ehpriv
  3c:	7c 18 cb c6 	dsn     r24,r25
  40:	7c 22 18 be 	lbepx   r1,r2,r3
  44:	7c 85 32 3e 	lhepx   r4,r5,r6
  48:	7c e8 48 3e 	lwepx   r7,r8,r9
  4c:	7d 4b 60 3a 	ldepx   r10,r11,r12
  50:	7d ae 7c be 	lfdepx  f13,r14,r15
  54:	7e 11 91 be 	stbepx  r16,r17,r18
  58:	7e 74 ab 3e 	sthepx  r19,r20,r21
  5c:	7e d7 c1 3e 	stwepx  r22,r23,r24
  60:	7f 3a d9 3a 	stdepx  r25,r26,r27
  64:	7f 9d f5 be 	stfdepx f28,r29,r30
  68:	7c 01 14 06 	lbdx    r0,r1,r2
  6c:	7d 8d 74 46 	lhdx    r12,r13,r14
  70:	7c 64 2c 86 	lwdx    r3,r4,r5
  74:	7f 5b e6 46 	lfddx   f26,r27,r28
  78:	7d f0 8c c6 	lddx    r15,r16,r17
  7c:	7c c7 45 06 	stbdx   r6,r7,r8
  80:	7e 53 a5 46 	sthdx   r18,r19,r20
  84:	7d 2a 5d 86 	stwdx   r9,r10,r11
  88:	7f be ff 46 	stfddx  f29,r30,r31
  8c:	7e b6 bd c6 	stddx   r21,r22,r23
  90:	7c 20 0d ec 	dcbal   r0,r1
  94:	7c 26 3f ec 	dcbzl   r6,r7
  98:	7c 1f 00 7e 	dcbstep r31,r0
  9c:	7c 01 10 fe 	dcbfep  r1,r2
  a0:	7c 64 29 fe 	dcbtstep r3,r4,r5
  a4:	7c c7 42 7e 	dcbtep  r6,r7,r8
  a8:	7c 0b 67 fe 	dcbzep  r11,r12
  ac:	7c 00 00 24 	tlbilxlpid
  b0:	7c 20 00 24 	tlbilxpid
  b4:	7c 62 18 24 	tlbilxva r2,r3
  b8:	7c 64 28 24 	tlbilxva r4,r5
