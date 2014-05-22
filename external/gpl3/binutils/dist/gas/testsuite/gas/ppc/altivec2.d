#as: -maltivec
#objdump: -dr -Maltivec
#name: Altivec ISA 2.07 instructions

.*

Disassembly of section \.text:

0+00 <start>:
   0:	(7c 60 e2 0e|0e e2 60 7c) 	lvepxl  v3,0,r28
   4:	(7e 64 92 0e|0e 92 64 7e) 	lvepxl  v19,r4,r18
   8:	(7f 60 9a 4e|4e 9a 60 7f) 	lvepx   v27,0,r19
   c:	(7c 39 92 4e|4e 92 39 7c) 	lvepx   v1,r25,r18
  10:	(7f e0 da 0a|0a da e0 7f) 	lvexbx  v31,0,r27
  14:	(7f 81 62 0a|0a 62 81 7f) 	lvexbx  v28,r1,r12
  18:	(7f e0 72 4a|4a 72 e0 7f) 	lvexhx  v31,0,r14
  1c:	(7e 30 fa 4a|4a fa 30 7e) 	lvexhx  v17,r16,r31
  20:	(7e c0 ea 8a|8a ea c0 7e) 	lvexwx  v22,0,r29
  24:	(7e f9 2a 8a|8a 2a f9 7e) 	lvexwx  v23,r25,r5
  28:	(7c 60 66 0a|0a 66 60 7c) 	lvsm    v3,0,r12
  2c:	(7f 7d 0e 0a|0a 0e 7d 7f) 	lvsm    v27,r29,r1
  30:	(7c e0 36 ca|ca 36 e0 7c) 	lvswxl  v7,0,r6
  34:	(7c f0 46 ca|ca 46 f0 7c) 	lvswxl  v7,r16,r8
  38:	(7d c0 94 ca|ca 94 c0 7d) 	lvswx   v14,0,r18
  3c:	(7f 9c 84 ca|ca 84 9c 7f) 	lvswx   v28,r28,r16
  40:	(7f 60 66 8a|8a 66 60 7f) 	lvtlxl  v27,0,r12
  44:	(7f 7c 06 8a|8a 06 7c 7f) 	lvtlxl  v27,r28,r0
  48:	(7e e0 cc 8a|8a cc e0 7e) 	lvtlx   v23,0,r25
  4c:	(7c 39 74 8a|8a 74 39 7c) 	lvtlx   v1,r25,r14
  50:	(7e 80 c6 4a|4a c6 80 7e) 	lvtrxl  v20,0,r24
  54:	(7e dd c6 4a|4a c6 dd 7e) 	lvtrxl  v22,r29,r24
  58:	(7f 00 44 4a|4a 44 00 7f) 	lvtrx   v24,0,r8
  5c:	(7d b7 e4 4a|4a e4 b7 7d) 	lvtrx   v13,r23,r28
  60:	(7d 9c 60 dc|dc 60 9c 7d) 	mvidsplt v12,r28,r12
  64:	(7d 5b 00 5c|5c 00 5b 7d) 	mviwsplt v10,r27,r0
  68:	(7f 60 6e 0e|0e 6e 60 7f) 	stvepxl v27,0,r13
  6c:	(7c 42 fe 0e|0e fe 42 7c) 	stvepxl v2,r2,r31
  70:	(7c 60 56 4e|4e 56 60 7c) 	stvepx  v3,0,r10
  74:	(7f 7c 06 4e|4e 06 7c 7f) 	stvepx  v27,r28,r0
  78:	(7d a0 33 0a|0a 33 a0 7d) 	stvexbx v13,0,r6
  7c:	(7d b9 1b 0a|0a 1b b9 7d) 	stvexbx v13,r25,r3
  80:	(7e c0 0b 4a|4a 0b c0 7e) 	stvexhx v22,0,r1
  84:	(7e 2e 53 4a|4a 53 2e 7e) 	stvexhx v17,r14,r10
  88:	(7e a0 db 8a|8a db a0 7e) 	stvexwx v21,0,r27
  8c:	(7f f2 0b 8a|8a 0b f2 7f) 	stvexwx v31,r18,r1
  90:	(7f 40 6f 8a|8a 6f 40 7f) 	stvflxl v26,0,r13
  94:	(7e cd af 8a|8a af cd 7e) 	stvflxl v22,r13,r21
  98:	(7c a0 4d 8a|8a 4d a0 7c) 	stvflx  v5,0,r9
  9c:	(7e b8 0d 8a|8a 0d b8 7e) 	stvflx  v21,r24,r1
  a0:	(7d a0 57 4a|4a 57 a0 7d) 	stvfrxl v13,0,r10
  a4:	(7d b1 cf 4a|4a cf b1 7d) 	stvfrxl v13,r17,r25
  a8:	(7e 20 55 4a|4a 55 20 7e) 	stvfrx  v17,0,r10
  ac:	(7d 0c fd 4a|4a fd 0c 7d) 	stvfrx  v8,r12,r31
  b0:	(7e 40 ef ca|ca ef 40 7e) 	stvswxl v18,0,r29
  b4:	(7f 4e 47 ca|ca 47 4e 7f) 	stvswxl v26,r14,r8
  b8:	(7c 00 7d ca|ca 7d 00 7c) 	stvswx  v0,0,r15
  bc:	(7d b7 3d ca|ca 3d b7 7d) 	stvswx  v13,r23,r7
  c0:	(10 d1 84 03|03 84 d1 10) 	vabsdub v6,v17,v16
  c4:	(12 b2 24 43|43 24 b2 12) 	vabsduh v21,v18,v4
  c8:	(13 34 4c 83|83 4c 34 13) 	vabsduw v25,v20,v9
#pass
