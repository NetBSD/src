#as:
#objdump: -dr
#name: allinsn

.*\.o:     file format elf32-epiphany

Disassembly of section .text:

00000000 \<bgt16\>:
   0:	0260      	bgt 4 \<bgt16\+0x4\>
   2:	0260      	bgt 6 \<bgt16\+0x6\>
   4:	fe60      	bgt 0 \<bgt16\>
   6:	fd60      	bgt 0 \<bgt16\>
   8:	0068 0000 	bgt 8 \<bgt16\+0x8\>
			8: R_EPIPHANY_SIMM24	\.data
   c:	0260      	bgt 10 \<bgt16\+0x10\>
   e:	f960      	bgt 0 \<bgt16\>
  10:	f860      	bgt 0 \<bgt16\>

00000012 \<bgtu16\>:
  12:	0220      	bgtu 16 \<bgtu16\+0x4\>
  14:	fe20      	bgtu 10 \<bgt16\+0x10\>
  16:	f520      	bgtu 0 \<bgt16\>
  18:	0220      	bgtu 1c \<bgtu16\+0xa\>
  1a:	fe20      	bgtu 16 \<bgtu16\+0x4\>
  1c:	f220      	bgtu 0 \<bgt16\>
  1e:	f120      	bgtu 0 \<bgt16\>
  20:	0220      	bgtu 24 \<bgte16\+0x2\>

00000022 \<bgte16\>:
  22:	ef70      	bgte 0 \<bgt16\>
  24:	ee70      	bgte 0 \<bgt16\>
  26:	ed70      	bgte 0 \<bgt16\>
  28:	ec70      	bgte 0 \<bgt16\>
  2a:	eb70      	bgte 0 \<bgt16\>
  2c:	fe70      	bgte 28 \<bgte16\+0x6\>
  2e:	0078 0000 	bgte 2e \<bgte16\+0xc\>
			2e: R_EPIPHANY_SIMM24	\.data
  32:	0078 0000 	bgte 32 \<bgte16\+0x10\>
			32: R_EPIPHANY_SIMM24	\.data

00000036 \<bgteu16\>:
  36:	0230      	bgteu 3a \<bgteu16\+0x4\>
  38:	fe30      	bgteu 34 \<bgte16\+0x12\>
  3a:	0038 0000 	bgteu 3a \<bgteu16\+0x4\>
			3a: R_EPIPHANY_SIMM24	\.data
  3e:	0230      	bgteu 42 \<bgteu16\+0xc\>
  40:	e030      	bgteu 0 \<bgt16\>
  42:	0230      	bgteu 46 \<bgteu16\+0x10\>
  44:	0038 0000 	bgteu 44 \<bgteu16\+0xe\>
			44: R_EPIPHANY_SIMM24	\.data
  48:	0038 0000 	bgteu 48 \<bgteu16\+0x12\>
			48: R_EPIPHANY_SIMM24	\.data

0000004c \<bgteu\>:
  4c:	fe80      	blt 48 \<bgteu16\+0x12\>
  4e:	0280      	blt 52 \<bgteu\+0x6\>
  50:	fe80      	blt 4c \<bgteu\>
  52:	0280      	blt 56 \<bgteu\+0xa\>
  54:	fe80      	blt 50 \<bgteu\+0x4\>
  56:	0280      	blt 5a \<bgteu\+0xe\>
  58:	0088 0000 	blt 58 \<bgteu\+0xc\>
			58: R_EPIPHANY_SIMM24	\.data
  5c:	0088 0000 	blt 5c \<bgteu\+0x10\>
			5c: R_EPIPHANY_SIMM24	\.data

00000060 \<blt\>:
  60:	fe50      	bltu 5c \<bgteu\+0x10\>
  62:	0250      	bltu 66 \<blt\+0x6\>
  64:	fe50      	bltu 60 \<blt\>
  66:	cd50      	bltu 0 \<bgt16\>
  68:	cc50      	bltu 0 \<bgt16\>
  6a:	cb50      	bltu 0 \<bgt16\>
  6c:	0250      	bltu 70 \<blt\+0x10\>
  6e:	0058 0000 	bltu 6e \<blt\+0xe\>
			6e: R_EPIPHANY_SIMM24	\.data

00000072 \<blte16\>:
  72:	c790      	blte 0 \<bgt16\>
  74:	0098 0000 	blte 74 \<blte16\+0x2\>
			74: R_EPIPHANY_SIMM24	\.data
  78:	0098 0000 	blte 78 \<blte16\+0x6\>
			78: R_EPIPHANY_SIMM24	\.data
  7c:	c290      	blte 0 \<bgt16\>
  7e:	fe90      	blte 7a \<blte16\+0x8\>
  80:	c090      	blte 0 \<bgt16\>
  82:	bf90      	blte 0 \<bgt16\>
  84:	0290      	blte 88 \<blte\+0x2\>

00000086 \<blte\>:
  86:	bd40      	blteu 0 \<bgt16\>
  88:	0048 0000 	blteu 88 \<blte\+0x2\>
			88: R_EPIPHANY_SIMM24	\.data
  8c:	ba40      	blteu 0 \<bgt16\>
  8e:	0048 0000 	blteu 8e \<blte\+0x8\>
			8e: R_EPIPHANY_SIMM24	\.data
  92:	b740      	blteu 0 \<bgt16\>
  94:	fe40      	blteu 90 \<blte\+0xa\>
  96:	0048 0000 	blteu 96 \<blte\+0x10\>
			96: R_EPIPHANY_SIMM24	\.data
  9a:	0048 0000 	blteu 9a \<blte\+0x14\>
			9a: R_EPIPHANY_SIMM24	\.data

0000009e \<bbeq16\>:
  9e:	b1a0      	bbeq 0 \<bgt16\>
  a0:	b0a0      	bbeq 0 \<bgt16\>
  a2:	00a8 0000 	bbeq a2 \<bbeq16\+0x4\>
			a2: R_EPIPHANY_SIMM24	\.data
  a6:	ada0      	bbeq 0 \<bgt16\>
  a8:	02a0      	bbeq ac \<bbeq16\+0xe\>
  aa:	00a8 0000 	bbeq aa \<bbeq16\+0xc\>
			aa: R_EPIPHANY_SIMM24	\.data
  ae:	00a8 0000 	bbeq ae \<bbeq16\+0x10\>
			ae: R_EPIPHANY_SIMM24	\.data
  b2:	02a0      	bbeq b6 \<bbeq\+0x2\>

000000b4 \<bbeq\>:
  b4:	00b8 0000 	bbne b4 \<bbeq\>
			b4: R_EPIPHANY_SIMM24	\.data
  b8:	feb0      	bbne b4 \<bbeq\>
  ba:	02b0      	bbne be \<bbeq\+0xa\>
  bc:	a2b0      	bbne 0 \<bgt16\>
  be:	02b0      	bbne c2 \<bbeq\+0xe\>
  c0:	02b0      	bbne c4 \<bbeq\+0x10\>
  c2:	9fb0      	bbne 0 \<bgt16\>
  c4:	9eb0      	bbne 0 \<bgt16\>

000000c6 \<bblt16\>:
  c6:	00c8 0000 	bblt c6 \<bblt16\>
			c6: R_EPIPHANY_SIMM24	\.data
  ca:	02c0      	bblt ce \<bblt16\+0x8\>
  cc:	02c0      	bblt d0 \<bblt16\+0xa\>
  ce:	02c0      	bblt d2 \<bblt16\+0xc\>
  d0:	fec0      	bblt cc \<bblt16\+0x6\>
  d2:	02c0      	bblt d6 \<bblt16\+0x10\>
  d4:	96c0      	bblt 0 \<bgt16\>
  d6:	fec0      	bblt d2 \<bblt16\+0xc\>

000000d8 \<bblt\>:
  d8:	02d0      	bblte dc \<bblt\+0x4\>
  da:	02d0      	bblte de \<bblt\+0x6\>
  dc:	92d0      	bblte 0 \<bgt16\>
  de:	91d0      	bblte 0 \<bgt16\>
  e0:	02d0      	bblte e4 \<bblt\+0xc\>
  e2:	fed0      	bblte de \<bblt\+0x6\>
  e4:	00d8 0000 	bblte e4 \<bblt\+0xc\>
			e4: R_EPIPHANY_SIMM24	\.data
  e8:	02d0      	bblte ec \<b16\+0x2\>

000000ea \<b16\>:
  ea:	8be0      	b 0 \<bgt16\>
  ec:	8ae0      	b 0 \<bgt16\>
  ee:	02e0      	b f2 \<b16\+0x8\>
  f0:	fee0      	b ec \<b16\+0x2\>
  f2:	87e0      	b 0 \<bgt16\>
  f4:	00e8 0000 	b f4 \<b16\+0xa\>
			f4: R_EPIPHANY_SIMM24	\.data
  f8:	00e8 0000 	b f8 \<b16\+0xe\>
			f8: R_EPIPHANY_SIMM24	\.data
  fc:	fee0      	b f8 \<b16\+0xe\>

000000fe \<b\>:
  fe:	fef0      	bl fa \<b16\+0x10\>
 100:	02f0      	bl 104 \<b\+0x6\>
 102:	7ff8 ffff 	bl 0 \<bgt16\>
 106:	fef0      	bl 102 \<b\+0x4\>
 108:	7cf8 ffff 	bl 0 \<bgt16\>
 10c:	fef0      	bl 108 \<b\+0xa\>
 10e:	fef0      	bl 10a \<b\+0xc\>
 110:	78f8 ffff 	bl 0 \<bgt16\>

00000114 \<bl\>:
 114:	114f 0402 	jr ip
 118:	0d42      	jr r3
 11a:	0142      	jr r0
 11c:	0d4f 0402 	jr fp
 120:	154f 0402 	jr sp
 124:	0142      	jr r0
 126:	0d42      	jr r3
 128:	0142      	jr r0

0000012a \<jr\>:
 12a:	114f 0402 	jr ip
 12e:	0d4f 1c02 	jr r59
 132:	114f 0c02 	jr r28
 136:	0d4f 0c02 	jr r27
 13a:	154f 0402 	jr sp
 13e:	0d4f 1802 	jr r51
 142:	014f 1c02 	jr r56
 146:	154f 1402 	jr r45

0000014a \<jalr16\>:
 14a:	115f 0402 	jalr ip
 14e:	0d52      	jalr r3
 150:	0152      	jalr r0
 152:	0d5f 0402 	jalr fp
 156:	155f 0402 	jalr sp
 15a:	0d52      	jalr r3
 15c:	0d5f 0402 	jalr fp
 160:	115f 0402 	jalr ip

00000164 \<jalr\>:
 164:	115f 0402 	jalr ip
 168:	0d5f 1c02 	jalr r59
 16c:	115f 0c02 	jalr r28
 170:	0d5f 0c02 	jalr r27
 174:	155f 0402 	jalr sp
 178:	0d5f 0402 	jalr fp
 17c:	115f 0c02 	jalr r28
 180:	0d5f 1c02 	jalr r59

00000184 \<ldrbx16\>:
 184:	9209 2480 	ldrb.l ip,\[ip,\+ip]
 188:	6d81      	ldrb r3,\[r3,r3]
 18a:	0001      	ldrb r0,\[r0,r0]
 18c:	6d89 2480 	ldrb.l fp,\[fp,\+fp]
 190:	b689 2480 	ldrb.l sp,\[sp,\+sp]
 194:	8009 2000 	ldrb.l ip,\[r0,\+r0]
 198:	6b09 0080 	ldrb.l r3,\[r2,\+lr]
 19c:	5189 0400 	ldrb.l r2,\[ip,\+r3]

000001a0 \<ldrbp16\>:
 1a0:	a18d 2080 	ldrb.l sp,\[r0],\+fp
 1a4:	c60d 2080 	ldrb.l lr,\[r1],\+ip
 1a8:	618d 2080 	ldrb.l fp,\[r0],\+fp

000001ac \<ldrbx\>:
 1ac:	9209 2480 	ldrb.l ip,\[ip,\+ip]
 1b0:	6d89 ff80 	ldrb.l r59,\[r59,\+r59]
 1b4:	9209 6d80 	ldrb.l r28,\[r28,\+r28]
 1b8:	6d89 6d80 	ldrb.l r27,\[r27,\+r27]
 1bc:	b689 2480 	ldrb.l sp,\[sp,\+sp]
 1c0:	2b89 aa80 	ldrb.l r41,\[r18,\+r47]
 1c4:	6289 a900 	ldrb.l r43,\[r16,\+r21]
 1c8:	0009 8480 	ldrb.l r32,\[r8,\+r8]

000001cc \<ldrbp\>:
 1cc:	850d 9900 	ldrb.l r36,\[r49],\+r18
 1d0:	0d0d 9f00 	ldrb.l r32,\[r59],\+r50
 1d4:	4c8d e580 	ldrb.l r58,\[fp],\+r25

000001d8 \<ldrbd16\>:
 1d8:	900c 2400 	ldrb ip,\[ip]
 1dc:	6f84      	ldrb r3,\[r3,0x7]
 1de:	0204      	ldrb r0,\[r0,0x4]
 1e0:	6d8c 2400 	ldrb.l fp,\[fp,\+0x3]
 1e4:	b48c 2400 	ldrb.l sp,\[sp,\+0x1]
 1e8:	d48c 2400 	ldrb.l lr,\[sp,\+0x1]
 1ec:	2004      	ldrb r1,\[r0]
 1ee:	2484      	ldrb r1,\[r1,0x1]

000001f0 \<ldrbd\>:
 1f0:	900c 2400 	ldrb ip,\[ip]
 1f4:	6f8c fcff 	ldrb.l r59,\[r59,\+0x7ff]
 1f8:	900c 6c80 	ldrb.l r28,\[r28,\+0x400]
 1fc:	6f8c 6c7f 	ldrb.l r27,\[r27,\+0x3ff]
 200:	b48c 2400 	ldrb.l sp,\[sp,\+0x1]
 204:	e70c 10c4 	ldrb.l r7,\[r33,\+0x626]
 208:	fa8c 60f4 	ldrb.l r31,\[r6,\+0x7a5]
 20c:	438c 20e4 	ldrb.l sl,\[r0,\+0x727]

00000210 \<ldrhx16\>:
 210:	9229 2480 	ldrh.l ip,\[ip,\+ip]
 214:	6da1      	ldrh r3,\[r3,r3]
 216:	0021      	ldrh r0,\[r0,r0]
 218:	6da9 2480 	ldrh.l fp,\[fp,\+fp]
 21c:	b6a9 2480 	ldrh.l sp,\[sp,\+sp]
 220:	0329 0080 	ldrh.l r0,\[r0,\+lr]
 224:	daa9 2480 	ldrh.l lr,\[lr,\+sp]
 228:	0da9 0480 	ldrh.l r0,\[fp,\+fp]

0000022c \<ldrhp16\>:
 22c:	55ad 0480 	ldrh.l r2,\[sp],\+fp
 230:	d5ad 4480 	ldrh.l r22,\[sp],\+fp

00000234 \<ldrhx\>:
 234:	9229 2480 	ldrh.l ip,\[ip,\+ip]
 238:	6da9 ff80 	ldrh.l r59,\[r59,\+r59]
 23c:	9229 6d80 	ldrh.l r28,\[r28,\+r28]
 240:	6da9 6d80 	ldrh.l r27,\[r27,\+r27]
 244:	b6a9 2480 	ldrh.l sp,\[sp,\+sp]
 248:	c6a9 a900 	ldrh.l r46,\[r17,\+r21]
 24c:	c7a9 6280 	ldrh.l r30,\[r1,\+r47]
 250:	6e29 a900 	ldrh.l r43,\[r19,\+r20]

00000254 \<ldrhp\>:
 254:	1ead 8d80 	ldrh.l r32,\[r31],\+r29
 258:	9d2d d480 	ldrh.l r52,\[r47],\+sl
 25c:	e1ad 7400 	ldrh.l r31,\[r40],\+r3

00000260 \<ldrhd16\>:
 260:	902c 2400 	ldrh ip,\[ip]
 264:	6fa4      	ldrh r3,\[r3,0x7]
 266:	0224      	ldrh r0,\[r0,0x4]
 268:	6dac 2400 	ldrh.l fp,\[fp,\+0x3]
 26c:	b4ac 2400 	ldrh.l sp,\[sp,\+0x1]
 270:	c82c 2000 	ldrh lr,\[r2]
 274:	63a4      	ldrh r3,\[r0,0x7]
 276:	0f24      	ldrh r0,\[r3,0x6]

00000278 \<ldrhd\>:
 278:	902c 2400 	ldrh ip,\[ip]
 27c:	6fac fcff 	ldrh.l r59,\[r59,\+0x7ff]
 280:	902c 6c80 	ldrh.l r28,\[r28,\+0x400]
 284:	6fac 6c7f 	ldrh.l r27,\[r27,\+0x3ff]
 288:	b4ac 2400 	ldrh.l sp,\[sp,\+0x1]
 28c:	a2ac ac98 	ldrh.l r45,\[r24,\+0x4c5]
 290:	8d2c 94d9 	ldrh.l r36,\[r43,\+0x6ca]
 294:	40ac b803 	ldrh.l r42,\[r48,\+0x19]

00000298 \<ldrx16\>:
 298:	9249 2480 	ldr.l ip,\[ip,\+ip]
 29c:	6dc1      	ldr r3,\[r3,r3]
 29e:	0041      	ldr r0,\[r0,r0]
 2a0:	6dc9 2480 	ldr.l fp,\[fp,\+fp]
 2a4:	b6c9 2480 	ldr.l sp,\[sp,\+sp]
 2a8:	6f49 0480 	ldr.l r3,\[fp,\+lr]
 2ac:	9949 2400 	ldr.l ip,\[lr,\+r2]
 2b0:	6b49 0080 	ldr.l r3,\[r2,\+lr]

000002b4 \<ldrp16\>:
 2b4:	cecd 2480 	ldr.l lr,\[fp],\+sp
 2b8:	144d 0400 	ldr.l r0,\[sp],\+r0
 2bc:	68cd 2000 	ldr.l fp,\[r2],\+r1

000002c0 \<ldrx\>:
 2c0:	9249 2480 	ldr.l ip,\[ip,\+ip]
 2c4:	6dc9 ff80 	ldr.l r59,\[r59,\+r59]
 2c8:	9249 6d80 	ldr.l r28,\[r28,\+r28]
 2cc:	6dc9 6d80 	ldr.l r27,\[r27,\+r27]
 2d0:	b6c9 2480 	ldr.l sp,\[sp,\+sp]
 2d4:	03c9 6a80 	ldr.l r24,\[r16,\+r47]
 2d8:	c4c9 5700 	ldr.l r22,\[r41,\+r49]
 2dc:	cfc9 2600 	ldr.l lr,\[fp,\+r39]

000002e0 \<ldrp\>:
 2e0:	b74d 4180 	ldr.l r21,\[r5],\+r30
 2e4:	934d 8480 	ldr.l r36,\[ip],\+lr
 2e8:	91cd 2080 	ldr.l ip,\[r4],\+fp

000002ec \<ldrd16\>:
 2ec:	904c 2400 	ldr ip,\[ip]
 2f0:	6fc4      	ldr r3,\[r3,0x7]
 2f2:	0244      	ldr r0,\[r0,0x4]
 2f4:	6dcc 2400 	ldr.l fp,\[fp,\+0x3]
 2f8:	b4cc 2400 	ldr.l sp,\[sp,\+0x1]
 2fc:	144c 0400 	ldr r0,\[sp]
 300:	87cc 2000 	ldr.l ip,\[r1,\+0x7]
 304:	64cc 2000 	ldr.l fp,\[r1,\+0x1]

00000308 \<ldrd\>:
 308:	904c 2400 	ldr ip,\[ip]
 30c:	6fcc fcff 	ldr.l r59,\[r59,\+0x7ff]
 310:	904c 6c80 	ldr.l r28,\[r28,\+0x400]
 314:	6fcc 6c7f 	ldr.l r27,\[r27,\+0x3ff]
 318:	b4cc 2400 	ldr.l sp,\[sp,\+0x1]
 31c:	dbcc 4c79 	ldr.l r22,\[r30,\+0x3cf]
 320:	f0cc 14aa 	ldr.l r7,\[r44,\+0x551]
 324:	efcc 48e7 	ldr.l r23,\[r19,\+0x73f]

00000328 \<ldrdx16\>:
 328:	9269 2480 	ldrd.l ip,\[ip,\+ip]
 32c:	8de1      	ldrd r4,\[r3,r3]
 32e:	0061      	ldrd r0,\[r0,r0]
 330:	cde9 2480 	ldrd.l lr,\[fp,\+fp]
 334:	16e9 4480 	ldrd.l r16,\[sp,\+sp]
 338:	ca69 6080 	ldrd.l r30,\[r2,\+ip]
 33c:	0de9 0400 	ldrd.l r0,\[fp,\+r3]
 340:	9369 4480 	ldrd.l r20,\[ip,\+lr]

00000344 \<ldrdp16\>:
 344:	8de5      	ldrd r4,\[r3],r3
 346:	0ded 4480 	ldrd.l r16,\[fp],\+fp
 34a:	96ed 4480 	ldrd.l r20,\[sp],\+sp
 34e:	50ed 2400 	ldrd.l sl,\[ip],\+r1
 352:	cf6d 6480 	ldrd.l r30,\[fp],\+lr
 356:	daed e480 	ldrd.l r62,\[lr],\+sp

0000035a \<ldrdx\>:
 35a:	9269 2480 	ldrd.l ip,\[ip,\+ip]
 35e:	4de9 ff80 	ldrd.l r58,\[r59,\+r59]
 362:	9269 6d80 	ldrd.l r28,\[r28,\+r28]
 366:	4de9 6d80 	ldrd.l r26,\[r27,\+r27]
 36a:	96e9 2480 	ldrd.l ip,\[sp,\+sp]
 36e:	0de9 8780 	ldrd.l r32,\[fp,\+r59]
 372:	8769 0800 	ldrd.l r4,\[r17,\+r6]
 376:	00e9 9400 	ldrd.l r32,\[r40,\+r1]

0000037a \<ldrdp\>:
 37a:	16ed 4480 	ldrd.l r16,\[sp],\+sp
 37e:	c76d b180 	ldrd.l r46,\[r33],\+r30
 382:	11ed 7380 	ldrd.l r24,\[r36],\+r59
 386:	41ed f080 	ldrd.l r58,\[r32],\+fp

0000038a \<ldrdd16\>:
 38a:	906c 2400 	ldrd ip,\[ip]
 38e:	8fe4      	ldrd r4,\[r3,0x7]
 390:	0264      	ldrd r0,\[r0,0x4]
 392:	0dec 4400 	ldrd.l r16,\[fp,\+0x3]
 396:	54ec 4400 	ldrd.l r18,\[sp,\+0x1]
 39a:	0dec 0400 	ldrd.l r0,\[fp,\+0x3]
 39e:	cfec 2400 	ldrd.l lr,\[fp,\+0x7]
 3a2:	d0ec 2400 	ldrd.l lr,\[ip,\+0x1]

000003a6 \<ldrdd\>:
 3a6:	906c 2400 	ldrd ip,\[ip]
 3aa:	4fec fcff 	ldrd.l r58,\[r59,\+0x7ff]
 3ae:	906c 6c80 	ldrd.l r28,\[r28,\+0x400]
 3b2:	4fec 0c7f 	ldrd.l r2,\[r27,\+0x3ff]
 3b6:	14ec 4400 	ldrd.l r16,\[sp,\+0x1]
 3ba:	94ec 085f 	ldrd.l r4,\[r21,\+0x2f9]
 3be:	c4ec 34c2 	ldrd.l lr,\[r41,\+0x611]
 3c2:	d96c 04f0 	ldrd.l r6,\[lr,\+0x782]

000003c6 \<strbx16\>:
 3c6:	9219 2480 	strb.l ip,\[ip,\+ip]
 3ca:	6d91      	strb r3,\[r3,r3]
 3cc:	0011      	strb r0,\[r0,r0]
 3ce:	6d99 2480 	strb.l fp,\[fp,\+fp]
 3d2:	b699 2480 	strb.l sp,\[sp,\+sp]
 3d6:	3999 0400 	strb.l r1,\[lr,\+r3]
 3da:	8f19 2080 	strb.l ip,\[r3,\+lr]
 3de:	d219 2480 	strb.l lr,\[ip,\+ip]

000003e2 \<strbx\>:
 3e2:	9219 2480 	strb.l ip,\[ip,\+ip]
 3e6:	6d99 ff80 	strb.l r59,\[r59,\+r59]
 3ea:	9219 6d80 	strb.l r28,\[r28,\+r28]
 3ee:	6d99 6d80 	strb.l r27,\[r27,\+r27]
 3f2:	b699 2480 	strb.l sp,\[sp,\+sp]
 3f6:	5e99 c480 	strb.l r50,\[r15,\+sp]
 3fa:	ce19 2700 	strb.l lr,\[fp,\+r52]
 3fe:	c199 2f00 	strb.l lr,\[r24,\+r51]

00000402 \<strbp16\>:
 402:	921d 2480 	strb.l ip,\[ip],\+ip
 406:	6d95      	strb r3,\[r3],r3
 408:	0015      	strb r0,\[r0],r0
 40a:	6d9d 2480 	strb.l fp,\[fp],\+fp
 40e:	b69d 2480 	strb.l sp,\[sp],\+sp
 412:	4e1d 0480 	strb.l r2,\[fp],\+ip
 416:	609d 2000 	strb.l fp,\[r0],\+r1
 41a:	4995      	strb r2,\[r2],r3

0000041c \<strbp\>:
 41c:	921d 2480 	strb.l ip,\[ip],\+ip
 420:	6d9d ff80 	strb.l r59,\[r59],\+r59
 424:	921d 6d80 	strb.l r28,\[r28],\+r28
 428:	6d9d 6d80 	strb.l r27,\[r27],\+r27
 42c:	b69d 2480 	strb.l sp,\[sp],\+sp
 430:	cd1d 3800 	strb.l lr,\[r51],\+r2
 434:	d11d 1700 	strb.l r6,\[r44],\+r50
 438:	849d a700 	strb.l r44,\[sb],\+r49

0000043c \<strbd16\>:
 43c:	901c 2400 	strb ip,\[ip]
 440:	6f94      	strb r3,\[r3,0x7]
 442:	0214      	strb r0,\[r0,0x4]
 444:	6d9c 2400 	strb.l fp,\[fp,\+0x3]
 448:	b49c 2400 	strb.l sp,\[sp,\+0x1]
 44c:	0894      	strb r0,\[r2,0x1]
 44e:	a99c 2000 	strb.l sp,\[r2,\+0x3]
 452:	6a1c 2000 	strb.l fp,\[r2,\+0x4]

00000456 \<strbd\>:
 456:	901c 2400 	strb ip,\[ip]
 45a:	6f9c fcff 	strb.l r59,\[r59,\+0x7ff]
 45e:	901c 6c80 	strb.l r28,\[r28,\+0x400]
 462:	6f9c 6c7f 	strb.l r27,\[r27,\+0x3ff]
 466:	b49c 2400 	strb.l sp,\[sp,\+0x1]
 46a:	ea1c 44af 	strb.l r23,\[sl,\+0x57c]
 46e:	8e9c 30b6 	strb.l ip,\[r35,\+0x5b5]
 472:	c91c dc88 	strb.l r54,\[r58,\+0x442]

00000476 \<strhx16\>:
 476:	9239 2480 	strh.l ip,\[ip,\+ip]
 47a:	6db1      	strh r3,\[r3,r3]
 47c:	0031      	strh r0,\[r0,r0]
 47e:	6db9 2480 	strh.l fp,\[fp,\+fp]
 482:	b6b9 2480 	strh.l sp,\[sp,\+sp]
 486:	0cb1      	strh r0,\[r3,r1]
 488:	2d39 0400 	strh.l r1,\[fp,\+r2]
 48c:	6db9 0080 	strh.l r3,\[r3,\+fp]

00000490 \<strhx\>:
 490:	9239 2480 	strh.l ip,\[ip,\+ip]
 494:	6db9 ff80 	strh.l r59,\[r59,\+r59]
 498:	9239 6d80 	strh.l r28,\[r28,\+r28]
 49c:	6db9 6d80 	strh.l r27,\[r27,\+r27]
 4a0:	b6b9 2480 	strh.l sp,\[sp,\+sp]
 4a4:	1bb9 5180 	strh.l r16,\[r38,\+r31]
 4a8:	1239 8580 	strh.l r32,\[ip,\+r28]
 4ac:	2cb9 e480 	strh.l r57,\[fp,\+sb]

000004b0 \<strhp16\>:
 4b0:	923d 2480 	strh.l ip,\[ip],\+ip
 4b4:	6db5      	strh r3,\[r3],r3
 4b6:	0035      	strh r0,\[r0],r0
 4b8:	6dbd 2480 	strh.l fp,\[fp],\+fp
 4bc:	b6bd 2480 	strh.l sp,\[sp],\+sp
 4c0:	0abd 0080 	strh.l r0,\[r2],\+sp
 4c4:	ac3d 2000 	strh.l sp,\[r3],\+r0
 4c8:	2035      	strh r1,\[r0],r0

000004ca \<strhp\>:
 4ca:	923d 2480 	strh.l ip,\[ip],\+ip
 4ce:	6dbd ff80 	strh.l r59,\[r59],\+r59
 4d2:	923d 6d80 	strh.l r28,\[r28],\+r28
 4d6:	6dbd 6d80 	strh.l r27,\[r27],\+r27
 4da:	b6bd 2480 	strh.l sp,\[sp],\+sp
 4de:	773d 1300 	strh.l r3,\[r37],\+r54
 4e2:	98bd 1980 	strh.l r4,\[r54],\+r25
 4e6:	a0bd 1180 	strh.l r5,\[r32],\+r25

000004ea \<strhd16\>:
 4ea:	903c 2400 	strh ip,\[ip]
 4ee:	6fb4      	strh r3,\[r3,0x7]
 4f0:	0234      	strh r0,\[r0,0x4]
 4f2:	6dbc 2400 	strh.l fp,\[fp,\+0x3]
 4f6:	b4bc 2400 	strh.l sp,\[sp,\+0x1]
 4fa:	61b4      	strh r3,\[r0,0x3]
 4fc:	d3bc 2400 	strh.l lr,\[ip,\+0x7]
 500:	6bb4      	strh r3,\[r2,0x7]

00000502 \<strhd\>:
 502:	903c 2400 	strh ip,\[ip]
 506:	6fbc fcff 	strh.l r59,\[r59,\+0x7ff]
 50a:	903c 6c80 	strh.l r28,\[r28,\+0x400]
 50e:	6fbc 6c7f 	strh.l r27,\[r27,\+0x3ff]
 512:	b4bc 2400 	strh.l sp,\[sp,\+0x1]
 516:	fabc 1093 	strh.l r7,\[r38,\+0x49d]
 51a:	32bc 6009 	strh.l r25,\[r4,\+0x4d]
 51e:	6fbc 244e 	strh.l fp,\[fp,\+0x277]

00000522 \<strx16\>:
 522:	9259 2480 	str.l ip,\[ip,\+ip]
 526:	6dd1      	str r3,\[r3,r3]
 528:	0051      	str r0,\[r0,r0]
 52a:	6dd9 2480 	str.l fp,\[fp,\+fp]
 52e:	b6d9 2480 	str.l sp,\[sp,\+sp]
 532:	cdd9 2000 	str.l lr,\[r3,\+r3]
 536:	6c59 0400 	str.l r3,\[fp,\+r0]
 53a:	94d9 2400 	str.l ip,\[sp,\+r1]

0000053e \<strx\>:
 53e:	9259 2480 	str.l ip,\[ip,\+ip]
 542:	6dd9 ff80 	str.l r59,\[r59,\+r59]
 546:	9259 6d80 	str.l r28,\[r28,\+r28]
 54a:	6dd9 6d80 	str.l r27,\[r27,\+r27]
 54e:	b6d9 2480 	str.l sp,\[sp,\+sp]
 552:	b659 cd80 	str.l r53,\[r29,\+r28]
 556:	d959 6a00 	str.l r30,\[r22,\+r34]
 55a:	9259 6e80 	str.l r28,\[r28,\+r44]

0000055e \<strp16\>:
 55e:	925d 2480 	str.l ip,\[ip],\+ip
 562:	6dd5      	str r3,\[r3],r3
 564:	0055      	str r0,\[r0],r0
 566:	6ddd 2480 	str.l fp,\[fp],\+fp
 56a:	b6dd 2480 	str.l sp,\[sp],\+sp
 56e:	c05d 2000 	str.l lr,\[r0],\+r0
 572:	62dd 2080 	str.l fp,\[r0],\+sp
 576:	6c5d 0400 	str.l r3,\[fp],\+r0

0000057a \<strp\>:
 57a:	925d 2480 	str.l ip,\[ip],\+ip
 57e:	6ddd ff80 	str.l r59,\[r59],\+r59
 582:	925d 6d80 	str.l r28,\[r28],\+r28
 586:	6ddd 6d80 	str.l r27,\[r27],\+r27
 58a:	b6dd 2480 	str.l sp,\[sp],\+sp
 58e:	d3dd 5080 	str.l r22,\[r36],\+r15
 592:	97dd a680 	str.l r44,\[sp],\+r47
 596:	62dd 5880 	str.l r19,\[r48],\+sp

0000059a \<strd16\>:
 59a:	905c 2400 	str ip,\[ip]
 59e:	6fd4      	str r3,\[r3,0x7]
 5a0:	0254      	str r0,\[r0,0x4]
 5a2:	6ddc 2400 	str.l fp,\[fp,\+0x3]
 5a6:	b4dc 2400 	str.l sp,\[sp,\+0x1]
 5aa:	6ddc 0400 	str.l r3,\[fp,\+0x3]
 5ae:	b35c 2400 	str.l sp,\[ip,\+0x6]
 5b2:	39dc 0400 	str.l r1,\[lr,\+0x3]

000005b6 \<strd\>:
 5b6:	905c 2400 	str ip,\[ip]
 5ba:	6fdc fcff 	str.l r59,\[r59,\+0x7ff]
 5be:	905c 6c80 	str.l r28,\[r28,\+0x400]
 5c2:	6fdc 6c7f 	str.l r27,\[r27,\+0x3ff]
 5c6:	b4dc 2400 	str.l sp,\[sp,\+0x1]
 5ca:	b15c b409 	str.l r45,\[r44,\+0x4a]
 5ce:	495c f82e 	str.l r58,\[r50,\+0x172]
 5d2:	0d5c a04e 	str.l r40,\[r3,\+0x272]

000005d6 \<strdx16\>:
 5d6:	9279 2480 	strd.l ip,\[ip,\+ip]
 5da:	4df1      	strd r2,\[r3,r3]
 5dc:	0071      	strd r0,\[r0,r0]
 5de:	0df9 4480 	strd.l r16,\[fp,\+fp]
 5e2:	56f9 4480 	strd.l r18,\[sp,\+sp]
 5e6:	8cf9 2000 	strd.l ip,\[r3,\+r1]
 5ea:	59f9 0480 	strd.l r2,\[lr,\+fp]
 5ee:	8979 2000 	strd.l ip,\[r2,\+r2]

000005f2 \<strdx\>:
 5f2:	9279 2480 	strd.l ip,\[ip,\+ip]
 5f6:	4df9 ff80 	strd.l r58,\[r59,\+r59]
 5fa:	9279 6d80 	strd.l r28,\[r28,\+r28]
 5fe:	4df9 6d80 	strd.l r26,\[r27,\+r27]
 602:	d6f9 2480 	strd.l lr,\[sp,\+sp]
 606:	d779 9880 	strd.l r38,\[r53,\+lr]
 60a:	0df9 6a80 	strd.l r24,\[r19,\+r43]
 60e:	8b79 2580 	strd.l ip,\[sl,\+r30]

00000612 \<strdp16\>:
 612:	927d 2480 	strd.l ip,\[ip],\+ip
 616:	4df5      	strd r2,\[r3],r3
 618:	0075      	strd r0,\[r0],r0
 61a:	cdfd 0480 	strd.l r6,\[fp],\+fp
 61e:	96fd 0480 	strd.l r4,\[sp],\+sp
 622:	4c75      	strd r2,\[r3],r0
 624:	40f5      	strd r2,\[r0],r1
 626:	58fd 0400 	strd.l r2,\[lr],\+r1

0000062a \<strdp\>:
 62a:	927d 2480 	strd.l ip,\[ip],\+ip
 62e:	4dfd ff80 	strd.l r58,\[r59],\+r59
 632:	927d 6d80 	strd.l r28,\[r28],\+r28
 636:	4dfd 6d80 	strd.l r26,\[r27],\+r27
 63a:	d6fd 4480 	strd.l r22,\[sp],\+sp
 63e:	ca7d 0680 	strd.l r6,\[sl],\+r44
 642:	4efd 3400 	strd.l sl,\[r43],\+r5
 646:	c77d a880 	strd.l r46,\[r17],\+lr

0000064a \<strdd16\>:
 64a:	107c 0400 	strd r0,\[ip]
 64e:	4ff4      	strd r2,\[r3,0x7]
 650:	0274      	strd r0,\[r0,0x4]
 652:	4dfc 0400 	strd.l r2,\[fp,\+0x3]
 656:	94fc 0400 	strd.l r4,\[sp,\+0x1]
 65a:	4af4      	strd r2,\[r2,0x5]
 65c:	cff4      	strd r6,\[r3,0x7]
 65e:	c574      	strd r6,\[r1,0x2]

00000660 \<strdd\>:
 660:	907c 2400 	strd ip,\[ip]
 664:	4ffc fcff 	strd.l r58,\[r59,\+0x7ff]
 668:	907c 6c80 	strd.l r28,\[r28,\+0x400]
 66c:	4ffc 6c7f 	strd.l r26,\[r27,\+0x3ff]
 670:	d4fc 2400 	strd.l lr,\[sp,\+0x1]
 674:	93fc 7859 	strd.l r28,\[r52,\+0x2cf]
 678:	157c b8f9 	strd.l r40,\[r53,\+0x7ca]
 67c:	877c bc3d 	strd.l r44,\[r57,\+0x1ee]

00000680 \<mov16EQ\>:
 680:	900f 2402 	moveq.l ip,ip
 684:	6c02      	moveq r3,r3
 686:	0002      	moveq r0,r0
 688:	6c0f 2402 	moveq.l fp,fp
 68c:	b40f 2402 	moveq.l sp,sp
 690:	880f 2002 	moveq.l ip,r2
 694:	4c0f 0402 	moveq.l r2,fp
 698:	740f 2402 	moveq.l fp,sp

0000069c \<movEQ\>:
 69c:	900f 2402 	moveq.l ip,ip
 6a0:	6c0f fc02 	moveq.l r59,r59
 6a4:	900f 6c02 	moveq.l r28,r28
 6a8:	6c0f 6c02 	moveq.l r27,r27
 6ac:	b40f 2402 	moveq.l sp,sp
 6b0:	180f 8c02 	moveq.l r32,r30
 6b4:	7c0f b002 	moveq.l r43,r39
 6b8:	240f 7002 	moveq.l r25,r33

000006bc \<mov16NE\>:
 6bc:	901f 2402 	movne.l ip,ip
 6c0:	6c12      	movne r3,r3
 6c2:	0012      	movne r0,r0
 6c4:	6c1f 2402 	movne.l fp,fp
 6c8:	b41f 2402 	movne.l sp,sp
 6cc:	6c12      	movne r3,r3
 6ce:	0c1f 0402 	movne.l r0,fp
 6d2:	6c1f 2402 	movne.l fp,fp

000006d6 \<movNE\>:
 6d6:	901f 2402 	movne.l ip,ip
 6da:	6c1f fc02 	movne.l r59,r59
 6de:	901f 6c02 	movne.l r28,r28
 6e2:	6c1f 6c02 	movne.l r27,r27
 6e6:	b41f 2402 	movne.l sp,sp
 6ea:	8c12      	movne r4,r3
 6ec:	8c1f 6402 	movne.l r28,fp
 6f0:	fc1f 5002 	movne.l r23,r39

000006f4 \<mov16GT\>:
 6f4:	906f 2402 	movgt.l ip,ip
 6f8:	6c62      	movgt r3,r3
 6fa:	0062      	movgt r0,r0
 6fc:	6c6f 2402 	movgt.l fp,fp
 700:	b46f 2402 	movgt.l sp,sp
 704:	2c62      	movgt r1,r3
 706:	cc6f 2002 	movgt.l lr,r3
 70a:	306f 0402 	movgt.l r1,ip

0000070e \<movGT\>:
 70e:	906f 2402 	movgt.l ip,ip
 712:	6c6f fc02 	movgt.l r59,r59
 716:	906f 6c02 	movgt.l r28,r28
 71a:	6c6f 6c02 	movgt.l r27,r27
 71e:	b46f 2402 	movgt.l sp,sp
 722:	346f 0802 	movgt.l r1,r21
 726:	ac6f 2002 	movgt.l sp,r3
 72a:	8c6f 7402 	movgt.l r28,r43

0000072e \<mov16GTU\>:
 72e:	902f 2402 	movgtu.l ip,ip
 732:	6c22      	movgtu r3,r3
 734:	0022      	movgtu r0,r0
 736:	6c2f 2402 	movgtu.l fp,fp
 73a:	b42f 2402 	movgtu.l sp,sp
 73e:	982f 2402 	movgtu.l ip,lr
 742:	b02f 2402 	movgtu.l sp,ip
 746:	942f 2402 	movgtu.l ip,sp

0000074a \<movGTU\>:
 74a:	902f 2402 	movgtu.l ip,ip
 74e:	6c2f fc02 	movgtu.l r59,r59
 752:	902f 6c02 	movgtu.l r28,r28
 756:	6c2f 6c02 	movgtu.l r27,r27
 75a:	b42f 2402 	movgtu.l sp,sp
 75e:	442f 9002 	movgtu.l r34,r33
 762:	202f 5802 	movgtu.l r17,r48
 766:	602f 8c02 	movgtu.l r35,r24

0000076a \<mov16GTE\>:
 76a:	907f 2402 	movgte.l ip,ip
 76e:	6c72      	movgte r3,r3
 770:	0072      	movgte r0,r0
 772:	6c7f 2402 	movgte.l fp,fp
 776:	b47f 2402 	movgte.l sp,sp
 77a:	0072      	movgte r0,r0
 77c:	547f 0402 	movgte.l r2,sp
 780:	c87f 2002 	movgte.l lr,r2

00000784 \<movGTE\>:
 784:	907f 2402 	movgte.l ip,ip
 788:	6c7f fc02 	movgte.l r59,r59
 78c:	907f 6c02 	movgte.l r28,r28
 790:	6c7f 6c02 	movgte.l r27,r27
 794:	b47f 2402 	movgte.l sp,sp
 798:	8c7f 3c02 	movgte.l ip,r59
 79c:	a87f 9402 	movgte.l r37,r42
 7a0:	887f ac02 	movgte.l r44,r26

000007a4 \<mov16GTEU\>:
 7a4:	903f 2402 	movgteu.l ip,ip
 7a8:	6c32      	movgteu r3,r3
 7aa:	0032      	movgteu r0,r0
 7ac:	6c3f 2402 	movgteu.l fp,fp
 7b0:	b43f 2402 	movgteu.l sp,sp
 7b4:	d03f 2402 	movgteu.l lr,ip
 7b8:	a43f 2002 	movgteu.l sp,r1
 7bc:	983f 2402 	movgteu.l ip,lr

000007c0 \<movGTEU\>:
 7c0:	903f 2402 	movgteu.l ip,ip
 7c4:	6c3f fc02 	movgteu.l r59,r59
 7c8:	903f 6c02 	movgteu.l r28,r28
 7cc:	6c3f 6c02 	movgteu.l r27,r27
 7d0:	b43f 2402 	movgteu.l sp,sp
 7d4:	5c3f f402 	movgteu.l r58,r47
 7d8:	143f e002 	movgteu.l r56,r5
 7dc:	903f 5802 	movgteu.l r20,r52

000007e0 \<mov16LT\>:
 7e0:	908f 2402 	movlt.l ip,ip
 7e4:	6c82      	movlt r3,r3
 7e6:	0082      	movlt r0,r0
 7e8:	6c8f 2402 	movlt.l fp,fp
 7ec:	b48f 2402 	movlt.l sp,sp
 7f0:	6c82      	movlt r3,r3
 7f2:	4882      	movlt r2,r2
 7f4:	988f 2402 	movlt.l ip,lr

000007f8 \<movLT\>:
 7f8:	908f 2402 	movlt.l ip,ip
 7fc:	6c8f fc02 	movlt.l r59,r59
 800:	908f 6c02 	movlt.l r28,r28
 804:	6c8f 6c02 	movlt.l r27,r27
 808:	b48f 2402 	movlt.l sp,sp
 80c:	908f c402 	movlt.l r52,ip
 810:	388f e802 	movlt.l r57,r22
 814:	1c8f 2002 	movlt.l r8,r7

00000818 \<mov16LTU\>:
 818:	905f 2402 	movltu.l ip,ip
 81c:	6c52      	movltu r3,r3
 81e:	0052      	movltu r0,r0
 820:	6c5f 2402 	movltu.l fp,fp
 824:	b45f 2402 	movltu.l sp,sp
 828:	885f 2002 	movltu.l ip,r2
 82c:	b05f 2402 	movltu.l sp,ip
 830:	2052      	movltu r1,r0

00000832 \<movLTU\>:
 832:	905f 2402 	movltu.l ip,ip
 836:	6c5f fc02 	movltu.l r59,r59
 83a:	905f 6c02 	movltu.l r28,r28
 83e:	6c5f 6c02 	movltu.l r27,r27
 842:	b45f 2402 	movltu.l sp,sp
 846:	bc5f 2c02 	movltu.l sp,r31
 84a:	705f a402 	movltu.l r43,ip
 84e:	e05f 1c02 	movltu.l r7,r56

00000852 \<mov16LTE\>:
 852:	909f 2402 	movlte.l ip,ip
 856:	6c92      	movlte r3,r3
 858:	0092      	movlte r0,r0
 85a:	6c9f 2402 	movlte.l fp,fp
 85e:	b49f 2402 	movlte.l sp,sp
 862:	0c92      	movlte r0,r3
 864:	709f 0402 	movlte.l r3,ip
 868:	789f 0402 	movlte.l r3,lr

0000086c \<movLTE\>:
 86c:	909f 2402 	movlte.l ip,ip
 870:	6c9f fc02 	movlte.l r59,r59
 874:	909f 6c02 	movlte.l r28,r28
 878:	6c9f 6c02 	movlte.l r27,r27
 87c:	b49f 2402 	movlte.l sp,sp
 880:	cc9f 6c02 	movlte.l r30,r27
 884:	709f 9802 	movlte.l r35,r52
 888:	f49f 3802 	movlte.l r15,r53

0000088c \<mov16LTEU\>:
 88c:	904f 2402 	movlteu.l ip,ip
 890:	6c42      	movlteu r3,r3
 892:	0042      	movlteu r0,r0
 894:	6c4f 2402 	movlteu.l fp,fp
 898:	b44f 2402 	movlteu.l sp,sp
 89c:	984f 2402 	movlteu.l ip,lr
 8a0:	4842      	movlteu r2,r2
 8a2:	4c4f 0402 	movlteu.l r2,fp

000008a6 \<movLTEU\>:
 8a6:	904f 2402 	movlteu.l ip,ip
 8aa:	6c4f fc02 	movlteu.l r59,r59
 8ae:	904f 6c02 	movlteu.l r28,r28
 8b2:	6c4f 6c02 	movlteu.l r27,r27
 8b6:	b44f 2402 	movlteu.l sp,sp
 8ba:	f04f 7002 	movlteu.l r31,r36
 8be:	084f 7802 	movlteu.l r24,r50
 8c2:	984f d802 	movlteu.l r52,r54

000008c6 \<mov16B\>:
 8c6:	90ef 2402 	mov.l ip,ip
 8ca:	6ce2      	mov r3,r3
 8cc:	00e2      	mov r0,r0
 8ce:	6cef 2402 	mov.l fp,fp
 8d2:	b4ef 2402 	mov.l sp,sp
 8d6:	84ef 2002 	mov.l ip,r1
 8da:	80ef 2002 	mov.l ip,r0
 8de:	10ef 0402 	mov.l r0,ip

000008e2 \<movB\>:
 8e2:	90ef 2402 	mov.l ip,ip
 8e6:	6cef fc02 	mov.l r59,r59
 8ea:	90ef 6c02 	mov.l r28,r28
 8ee:	6cef 6c02 	mov.l r27,r27
 8f2:	b4ef 2402 	mov.l sp,sp
 8f6:	2cef 1c02 	mov.l r1,r59
 8fa:	90ef 6402 	mov.l r28,ip
 8fe:	a8ef 1402 	mov.l r5,r42

00000902 \<mov16BEQ\>:
 902:	90af 2402 	movbeq.l ip,ip
 906:	6ca2      	movbeq r3,r3
 908:	00a2      	movbeq r0,r0
 90a:	6caf 2402 	movbeq.l fp,fp
 90e:	b4af 2402 	movbeq.l sp,sp
 912:	c8af 2002 	movbeq.l lr,r2
 916:	68af 2002 	movbeq.l fp,r2
 91a:	84af 2002 	movbeq.l ip,r1

0000091e \<movBEQ\>:
 91e:	90af 2402 	movbeq.l ip,ip
 922:	6caf fc02 	movbeq.l r59,r59
 926:	90af 6c02 	movbeq.l r28,r28
 92a:	6caf 6c02 	movbeq.l r27,r27
 92e:	b4af 2402 	movbeq.l sp,sp
 932:	a0af 6802 	movbeq.l r29,r16
 936:	58af 5402 	movbeq.l r18,r46
 93a:	c4af 2002 	movbeq.l lr,r1

0000093e \<mov16BNE\>:
 93e:	90bf 2402 	movbne.l ip,ip
 942:	6cb2      	movbne r3,r3
 944:	00b2      	movbne r0,r0
 946:	6cbf 2402 	movbne.l fp,fp
 94a:	b4bf 2402 	movbne.l sp,sp
 94e:	28b2      	movbne r1,r2
 950:	84bf 2002 	movbne.l ip,r1
 954:	8cbf 2002 	movbne.l ip,r3

00000958 \<movBNE\>:
 958:	90bf 2402 	movbne.l ip,ip
 95c:	6cbf fc02 	movbne.l r59,r59
 960:	90bf 6c02 	movbne.l r28,r28
 964:	6cbf 6c02 	movbne.l r27,r27
 968:	b4bf 2402 	movbne.l sp,sp
 96c:	fcbf 2002 	movbne.l r15,r7
 970:	0cbf 7402 	movbne.l r24,r43
 974:	f0bf 5802 	movbne.l r23,r52

00000978 \<mov16BLT\>:
 978:	90cf 2402 	movblt.l ip,ip
 97c:	6cc2      	movblt r3,r3
 97e:	00c2      	movblt r0,r0
 980:	6ccf 2402 	movblt.l fp,fp
 984:	b4cf 2402 	movblt.l sp,sp
 988:	b8cf 2402 	movblt.l sp,lr
 98c:	98cf 2402 	movblt.l ip,lr
 990:	d4cf 2402 	movblt.l lr,sp

00000994 \<movBLT\>:
 994:	90cf 2402 	movblt.l ip,ip
 998:	6ccf fc02 	movblt.l r59,r59
 99c:	90cf 6c02 	movblt.l r28,r28
 9a0:	6ccf 6c02 	movblt.l r27,r27
 9a4:	b4cf 2402 	movblt.l sp,sp
 9a8:	90cf d402 	movblt.l r52,r44
 9ac:	2ccf f002 	movblt.l r57,r35
 9b0:	a4cf d002 	movblt.l r53,r33

000009b4 \<mov16BLTE\>:
 9b4:	90df 2402 	movblte.l ip,ip
 9b8:	6cd2      	movblte r3,r3
 9ba:	00d2      	movblte r0,r0
 9bc:	6cdf 2402 	movblte.l fp,fp
 9c0:	b4df 2402 	movblte.l sp,sp
 9c4:	b0df 2402 	movblte.l sp,ip
 9c8:	0cdf 0402 	movblte.l r0,fp
 9cc:	14df 0402 	movblte.l r0,sp

000009d0 \<movBLTE\>:
 9d0:	90df 2402 	movblte.l ip,ip
 9d4:	6cdf fc02 	movblte.l r59,r59
 9d8:	90df 6c02 	movblte.l r28,r28
 9dc:	6cdf 6c02 	movblte.l r27,r27
 9e0:	b4df 2402 	movblte.l sp,sp
 9e4:	50df f402 	movblte.l r58,r44
 9e8:	78df 8802 	movblte.l r35,r22
 9ec:	08df 2002 	movblte.l r8,r2

000009f0 \<movts16\>:
 9f0:	810f 2002 	movts.l config,ip
 9f4:	750f 0402 	movts.l ipend,r3
 9f8:	010f 0402 	movts.l iret,r0
 9fc:	6d0f 2002 	movts.l debug,fp
 a00:	a50f 2002 	movts.l status,sp
 a04:	650f 2002 	movts.l status,fp
 a08:	690f 2002 	movts.l pc,fp
 a0c:	050f 0402 	movts.l imask,r0

00000a10 \<movts\>:
 a10:	810f 2002 	movts.l config,ip
 a14:	750f e402 	movts.l ipend,r59
 a18:	810f 6402 	movts.l iret,r28
 a1c:	6d0f 6002 	movts.l debug,r27
 a20:	a50f 2002 	movts.l status,sp
 a24:	4d0f c002 	movts.l debug,r50
 a28:	350f 8402 	movts.l ipend,r33
 a2c:	850f 2002 	movts.l status,ip

00000a30 \<movfs16\>:
 a30:	811f 2002 	movfs.l ip,config
 a34:	751f 0402 	movfs.l r3,ipend
 a38:	011f 0402 	movfs.l r0,iret
 a3c:	6d1f 2002 	movfs.l fp,debug
 a40:	a51f 2002 	movfs.l sp,status
 a44:	211f 0402 	movfs.l r1,iret
 a48:	4512      	movfs r2,status
 a4a:	cd1f 2002 	movfs.l lr,debug

00000a4e \<movfs\>:
 a4e:	811f 2002 	movfs.l ip,config
 a52:	751f e402 	movfs.l r59,ipend
 a56:	811f 6402 	movfs.l r28,iret
 a5a:	6d1f 6002 	movfs.l r27,debug
 a5e:	a51f 2002 	movfs.l sp,status
 a62:	ad1f 2002 	movfs.l sp,debug
 a66:	e51f 2002 	movfs.l r15,status
 a6a:	051f 4402 	movfs.l r16,imask

00000a6e \<nop\>:
 a6e:	01a2      	nop

00000a70 \<idle\>:
 a70:	01b2      	idle

00000a72 \<bkpt\>:
 a72:	01c2      	bkpt

00000a74 \<rti\>:
 a74:	01d2      	rti

00000a76 \<trap16\>:
 a76:	03e2      	trap 0x0
 a78:	1fe2      	trap 0x7
 a7a:	13e2      	trap 0x4
 a7c:	0fe2      	trap 0x3
 a7e:	07e2      	trap 0x1
 a80:	1be2      	trap 0x6
 a82:	0fe2      	trap 0x3
 a84:	17e2      	trap 0x5

00000a86 \<add16\>:
 a86:	921f 248a 	add.l ip,ip,ip
 a8a:	6d9a      	add r3,r3,r3
 a8c:	001a      	add r0,r0,r0
 a8e:	6d9f 248a 	add.l fp,fp,fp
 a92:	b69f 248a 	add.l sp,sp,sp
 a96:	ab1f 208a 	add.l sp,r2,lr
 a9a:	089a      	add r0,r2,r1
 a9c:	8d9f 248a 	add.l ip,fp,fp

00000aa0 \<add\>:
 aa0:	921f 248a 	add.l ip,ip,ip
 aa4:	6d9f ff8a 	add.l r59,r59,r59
 aa8:	921f 6d8a 	add.l r28,r28,r28
 aac:	6d9f 6d8a 	add.l r27,r27,r27
 ab0:	b69f 248a 	add.l sp,sp,sp
 ab4:	081f e50a 	add.l r56,sl,r16
 ab8:	851f 8e0a 	add.l r36,r25,r34
 abc:	449f 190a 	add.l r2,r49,r17

00000ac0 \<sub16\>:
 ac0:	923f 248a 	sub.l ip,ip,ip
 ac4:	6dba      	sub r3,r3,r3
 ac6:	003a      	sub r0,r0,r0
 ac8:	6dbf 248a 	sub.l fp,fp,fp
 acc:	b6bf 248a 	sub.l sp,sp,sp
 ad0:	533f 048a 	sub.l r2,ip,lr
 ad4:	d83f 240a 	sub.l lr,lr,r0
 ad8:	6dba      	sub r3,r3,r3

00000ada \<sub\>:
 ada:	923f 248a 	sub.l ip,ip,ip
 ade:	6dbf ff8a 	sub.l r59,r59,r59
 ae2:	923f 6d8a 	sub.l r28,r28,r28
 ae6:	6dbf 6d8a 	sub.l r27,r27,r27
 aea:	b6bf 248a 	sub.l sp,sp,sp
 aee:	9a3f 250a 	sub.l ip,lr,r20
 af2:	1bbf ca8a 	sub.l r48,r22,r47
 af6:	62bf 588a 	sub.l r19,r48,sp

00000afa \<and16\>:
 afa:	925f 248a 	and.l ip,ip,ip
 afe:	6dda      	and r3,r3,r3
 b00:	005a      	and r0,r0,r0
 b02:	6ddf 248a 	and.l fp,fp,fp
 b06:	b6df 248a 	and.l sp,sp,sp
 b0a:	75df 240a 	and.l fp,sp,r3
 b0e:	6dda      	and r3,r3,r3
 b10:	96df 248a 	and.l ip,sp,sp

00000b14 \<and\>:
 b14:	925f 248a 	and.l ip,ip,ip
 b18:	6ddf ff8a 	and.l r59,r59,r59
 b1c:	925f 6d8a 	and.l r28,r28,r28
 b20:	6ddf 6d8a 	and.l r27,r27,r27
 b24:	b6df 248a 	and.l sp,sp,sp
 b28:	935f c68a 	and.l r52,ip,r46
 b2c:	825f b68a 	and.l r44,r40,r44
 b30:	0bdf 7d8a 	and.l r24,r58,r31

00000b34 \<orr16\>:
 b34:	927f 248a 	orr.l ip,ip,ip
 b38:	6dfa      	orr r3,r3,r3
 b3a:	007a      	orr r0,r0,r0
 b3c:	6dff 248a 	orr.l fp,fp,fp
 b40:	b6ff 248a 	orr.l sp,sp,sp
 b44:	c6ff 208a 	orr.l lr,r1,sp
 b48:	7b7f 048a 	orr.l r3,lr,lr
 b4c:	4d7a      	orr r2,r3,r2

00000b4e \<orr\>:
 b4e:	927f 248a 	orr.l ip,ip,ip
 b52:	6dff ff8a 	orr.l r59,r59,r59
 b56:	927f 6d8a 	orr.l r28,r28,r28
 b5a:	6dff 6d8a 	orr.l r27,r27,r27
 b5e:	b6ff 248a 	orr.l sp,sp,sp
 b62:	95ff c38a 	orr.l r52,r5,r59
 b66:	e1ff 328a 	orr.l r15,r32,r43
 b6a:	167f ee8a 	orr.l r56,r29,r44

00000b6e \<eor16\>:
 b6e:	920f 248a 	eor.l ip,ip,ip
 b72:	6d8a      	eor r3,r3,r3
 b74:	000a      	eor r0,r0,r0
 b76:	6d8f 248a 	eor.l fp,fp,fp
 b7a:	b68f 248a 	eor.l sp,sp,sp
 b7e:	8d0f 200a 	eor.l ip,r3,r2
 b82:	750f 040a 	eor.l r3,sp,r2
 b86:	750f 240a 	eor.l fp,sp,r2

00000b8a \<eor\>:
 b8a:	920f 248a 	eor.l ip,ip,ip
 b8e:	6d8f ff8a 	eor.l r59,r59,r59
 b92:	920f 6d8a 	eor.l r28,r28,r28
 b96:	6d8f 6d8a 	eor.l r27,r27,r27
 b9a:	b68f 248a 	eor.l sp,sp,sp
 b9e:	228f 5d8a 	eor.l r17,r56,r29
 ba2:	a58f 358a 	eor.l sp,r41,r27
 ba6:	698f 268a 	eor.l fp,sl,r43

00000baa \<asr16\>:
 baa:	926f 248a 	asr.l ip,ip,ip
 bae:	6dea      	asr r3,r3,r3
 bb0:	006a      	asr r0,r0,r0
 bb2:	6def 248a 	asr.l fp,fp,fp
 bb6:	b6ef 248a 	asr.l sp,sp,sp
 bba:	61ea      	asr r3,r0,r3
 bbc:	676f 008a 	asr.l r3,r1,lr
 bc0:	0eef 048a 	asr.l r0,fp,sp

00000bc4 \<asr\>:
 bc4:	926f 248a 	asr.l ip,ip,ip
 bc8:	6def ff8a 	asr.l r59,r59,r59
 bcc:	926f 6d8a 	asr.l r28,r28,r28
 bd0:	6def 6d8a 	asr.l r27,r27,r27
 bd4:	b6ef 248a 	asr.l sp,sp,sp
 bd8:	44ef 858a 	asr.l r34,sb,r25
 bdc:	64ef ca0a 	asr.l r51,r17,r33
 be0:	9def 208a 	asr.l ip,r7,fp

00000be4 \<lsr16\>:
 be4:	924f 248a 	lsr.l ip,ip,ip
 be8:	6dca      	lsr r3,r3,r3
 bea:	004a      	lsr r0,r0,r0
 bec:	6dcf 248a 	lsr.l fp,fp,fp
 bf0:	b6cf 248a 	lsr.l sp,sp,sp
 bf4:	adcf 208a 	lsr.l sp,r3,fp
 bf8:	674f 208a 	lsr.l fp,r1,lr
 bfc:	c94f 200a 	lsr.l lr,r2,r2

00000c00 \<lsr\>:
 c00:	924f 248a 	lsr.l ip,ip,ip
 c04:	6dcf ff8a 	lsr.l r59,r59,r59
 c08:	924f 6d8a 	lsr.l r28,r28,r28
 c0c:	6dcf 6d8a 	lsr.l r27,r27,r27
 c10:	b6cf 248a 	lsr.l sp,sp,sp
 c14:	c5cf 0d0a 	lsr.l r6,r25,r19
 c18:	984f 3a0a 	lsr.l ip,r54,r32
 c1c:	b64f 248a 	lsr.l sp,sp,ip

00000c20 \<lsl16\>:
 c20:	922f 248a 	lsl.l ip,ip,ip
 c24:	6daa      	lsl r3,r3,r3
 c26:	002a      	lsl r0,r0,r0
 c28:	6daf 248a 	lsl.l fp,fp,fp
 c2c:	b6af 248a 	lsl.l sp,sp,sp
 c30:	922f 248a 	lsl.l ip,ip,ip
 c34:	c62f 208a 	lsl.l lr,r1,ip
 c38:	d5af 240a 	lsl.l lr,sp,r3

00000c3c \<lsl\>:
 c3c:	922f 248a 	lsl.l ip,ip,ip
 c40:	6daf ff8a 	lsl.l r59,r59,r59
 c44:	922f 6d8a 	lsl.l r28,r28,r28
 c48:	6daf 6d8a 	lsl.l r27,r27,r27
 c4c:	b6af 248a 	lsl.l sp,sp,sp
 c50:	8faf 948a 	lsl.l r36,r43,r15
 c54:	5eaf 920a 	lsl.l r34,r39,r37
 c58:	e6af 518a 	lsl.l r23,r33,r29

00000c5c \<addi16\>:
 c5c:	901b 2400 	add ip,ip,0
 c60:	6f9b 0000 	add r3,r3,7
 c64:	021b 0000 	add r0,r0,4
 c68:	6d9b 2400 	add fp,fp,3
 c6c:	b49b 2400 	add sp,sp,1
 c70:	6493      	add r3,r1,1
 c72:	2d9b 0400 	add r1,fp,3
 c76:	0f9b 0400 	add r0,fp,7

00000c7a \<addi\>:
 c7a:	901b 2400 	add ip,ip,0
 c7e:	6f9b fc7f 	add r59,r59,1023
 c82:	939b 6c04 	add r28,r28,39
 c86:	6f9b 6c7f 	add r27,r27,1023
 c8a:	b49b 2400 	add sp,sp,1
 c8e:	329b cc14 	add r49,r28,165
 c92:	eb9b 604d 	add r31,r2,623
 c96:	049b 4476 	add r16,sb,945

00000c9a \<subi16\>:
 c9a:	903b 2400 	sub ip,ip,0
 c9e:	6fbb 0000 	sub r3,r3,7
 ca2:	023b 0000 	sub r0,r0,4
 ca6:	6dbb 2400 	sub fp,fp,3
 caa:	b4bb 2400 	sub sp,sp,1
 cae:	8d3b 2000 	sub ip,r3,2
 cb2:	ce3b 2000 	sub lr,r3,4
 cb6:	88bb 2000 	sub ip,r2,1

00000cba \<subi\>:
 cba:	903b 2400 	sub ip,ip,0
 cbe:	6cbb fc00 	sub r59,r59,1
 cc2:	93bb 6c7f 	sub r28,r28,1023
 cc6:	6f3b 6c7f 	sub r27,r27,1022
 cca:	b4bb 2400 	sub sp,sp,1
 cce:	7a3b c068 	sub r51,r6,836
 cd2:	e23b b460 	sub r47,r40,772
 cd6:	f03b c03d 	sub r55,r4,488

00000cda \<lsri16\>:
 cda:	900f 2406 	lsr.l ip,ip,0x0
 cde:	6fe6      	lsr r3,r3,0x1f
 ce0:	0206      	lsr r0,r0,0x10
 ce2:	6def 2406 	lsr.l fp,fp,0xf
 ce6:	b42f 2406 	lsr.l sp,sp,0x1
 cea:	0cc6      	lsr r0,r3,0x6
 cec:	2906      	lsr r1,r2,0x8
 cee:	79cf 2406 	lsr.l fp,lr,0xe

00000cf2 \<lsri32\>:
 cf2:	900f 2406 	lsr.l ip,ip,0x0
 cf6:	6fef fc06 	lsr.l r59,r59,0x1f
 cfa:	920f 6c06 	lsr.l r28,r28,0x10
 cfe:	6def 6c06 	lsr.l r27,r27,0xf
 d02:	b42f 2406 	lsr.l sp,sp,0x1
 d06:	c26f 7806 	lsr.l r30,r48,0x13
 d0a:	7eef a006 	lsr.l r43,r7,0x17
 d0e:	8b8f 6006 	lsr.l r28,r2,0x1c

00000d12 \<lsli16\>:
 d12:	901f 2406 	lsl.l ip,ip,0x0
 d16:	6ff6      	lsl r3,r3,0x1f
 d18:	0216      	lsl r0,r0,0x10
 d1a:	6dff 2406 	lsl.l fp,fp,0xf
 d1e:	b43f 2406 	lsl.l sp,sp,0x1
 d22:	4d76      	lsl r2,r3,0xb
 d24:	c8df 2006 	lsl.l lr,r2,0x6
 d28:	0a16      	lsl r0,r2,0x10

00000d2a \<lsli32\>:
 d2a:	901f 2406 	lsl.l ip,ip,0x0
 d2e:	6fff fc06 	lsl.l r59,r59,0x1f
 d32:	921f 6c06 	lsl.l r28,r28,0x10
 d36:	6dff 6c06 	lsl.l r27,r27,0xf
 d3a:	b43f 2406 	lsl.l sp,sp,0x1
 d3e:	0e7f f806 	lsl.l r56,r51,0x13
 d42:	3e7f 5006 	lsl.l r17,r39,0x13
 d46:	519f 0406 	lsl.l r2,ip,0xc

00000d4a \<asri16\>:
 d4a:	900f 240e 	asr.l ip,ip,0x0
 d4e:	6fee      	asr r3,r3,0x1f
 d50:	020e      	asr r0,r0,0x10
 d52:	6def 240e 	asr.l fp,fp,0xf
 d56:	b42f 240e 	asr.l sp,sp,0x1
 d5a:	d2af 240e 	asr.l lr,ip,0x15
 d5e:	6ece      	asr r3,r3,0x16
 d60:	6d2e      	asr r3,r3,0x9

00000d62 \<asri32\>:
 d62:	900f 240e 	asr.l ip,ip,0x0
 d66:	6fef fc0e 	asr.l r59,r59,0x1f
 d6a:	920f 6c0e 	asr.l r28,r28,0x10
 d6e:	6def 6c0e 	asr.l r27,r27,0xf
 d72:	b42f 240e 	asr.l sp,sp,0x1
 d76:	9a2f d40e 	asr.l r52,r46,0x11
 d7a:	e2cf 5c0e 	asr.l r23,r56,0x16
 d7e:	bb8f 540e 	asr.l r21,r46,0x1c

00000d82 \<mov8\>:
 d82:	800b 2002 	mov ip,0x0
 d86:	7fe3      	mov r3,0xff
 d88:	1003      	mov r0,0x80
 d8a:	6feb 2002 	mov fp,0x7f
 d8e:	a02b 2002 	mov sp,0x1
 d92:	cb6b 2002 	mov lr,0x5b
 d96:	09a3      	mov r0,0x4d
 d98:	614b 2002 	mov fp,0xa

00000d9c \<mov16\>:
 d9c:	800b 2002 	mov ip,0x0
 da0:	7feb eff2 	mov r59,0xffff
 da4:	800b 6802 	mov r28,0x8000
 da8:	7feb 67f2 	mov r27,0x7fff
 dac:	a02b 2002 	mov sp,0x1
 db0:	be2b cee2 	mov r53,0xeef1
 db4:	5deb 4cb2 	mov r18,0xcbef
 db8:	044b 48e2 	mov r16,0x8e22

00000dbc \<faddf16\>:
 dbc:	920f 2487 	fadd.l ip,ip,ip
 dc0:	6d87      	fadd r3,r3,r3
 dc2:	0007      	fadd r0,r0,r0
 dc4:	6d8f 2487 	fadd.l fp,fp,fp
 dc8:	b68f 2487 	fadd.l sp,sp,sp
 dcc:	b10f 2407 	fadd.l sp,ip,r2
 dd0:	a90f 2007 	fadd.l sp,r2,r2
 dd4:	b98f 2487 	fadd.l sp,lr,fp

00000dd8 \<faddf32\>:
 dd8:	920f 2487 	fadd.l ip,ip,ip
 ddc:	6d8f ff87 	fadd.l r59,r59,r59
 de0:	920f 6d87 	fadd.l r28,r28,r28
 de4:	6d8f 6d87 	fadd.l r27,r27,r27
 de8:	b68f 2487 	fadd.l sp,sp,sp
 dec:	b78f 2e07 	fadd.l sp,r29,r39
 df0:	018f 9407 	fadd.l r32,r40,r3
 df4:	170f ac87 	fadd.l r40,r29,lr

00000df8 \<fsubf16\>:
 df8:	921f 2487 	fsub.l ip,ip,ip
 dfc:	6d97      	fsub r3,r3,r3
 dfe:	0017      	fsub r0,r0,r0
 e00:	6d9f 2487 	fsub.l fp,fp,fp
 e04:	b69f 2487 	fsub.l sp,sp,sp
 e08:	5a9f 0487 	fsub.l r2,lr,sp
 e0c:	661f 0087 	fsub.l r3,r1,ip
 e10:	711f 0407 	fsub.l r3,ip,r2

00000e14 \<fsubf32\>:
 e14:	921f 2487 	fsub.l ip,ip,ip
 e18:	6d9f ff87 	fsub.l r59,r59,r59
 e1c:	921f 6d87 	fsub.l r28,r28,r28
 e20:	6d9f 6d87 	fsub.l r27,r27,r27
 e24:	b69f 2487 	fsub.l sp,sp,sp
 e28:	219f 1c87 	fsub.l r1,r56,fp
 e2c:	7b9f 0887 	fsub.l r3,r22,r15
 e30:	c29f 1a87 	fsub.l r6,r48,r45

00000e34 \<fmulf16\>:
 e34:	922f 2487 	fmul.l ip,ip,ip
 e38:	6da7      	fmul r3,r3,r3
 e3a:	0027      	fmul r0,r0,r0
 e3c:	6daf 2487 	fmul.l fp,fp,fp
 e40:	b6af 2487 	fmul.l sp,sp,sp
 e44:	71af 0487 	fmul.l r3,ip,fp
 e48:	c52f 2007 	fmul.l lr,r1,r2
 e4c:	bb2f 2487 	fmul.l sp,lr,lr

00000e50 \<fmulf32\>:
 e50:	922f 2487 	fmul.l ip,ip,ip
 e54:	6daf ff87 	fmul.l r59,r59,r59
 e58:	922f 6d87 	fmul.l r28,r28,r28
 e5c:	6daf 6d87 	fmul.l r27,r27,r27
 e60:	b6af 2487 	fmul.l sp,sp,sp
 e64:	5daf eb07 	fmul.l r58,r23,r51
 e68:	cbaf 4287 	fmul.l r22,r2,r47
 e6c:	d92f a487 	fmul.l r46,lr,sl

00000e70 \<fmaddf16\>:
 e70:	923f 2487 	fmadd.l ip,ip,ip
 e74:	6db7      	fmadd r3,r3,r3
 e76:	0037      	fmadd r0,r0,r0
 e78:	6dbf 2487 	fmadd.l fp,fp,fp
 e7c:	b6bf 2487 	fmadd.l sp,sp,sp
 e80:	a5bf 2007 	fmadd.l sp,r1,r3
 e84:	6c37      	fmadd r3,r3,r0
 e86:	523f 0487 	fmadd.l r2,ip,ip

00000e8a \<fmaddf32\>:
 e8a:	923f 2487 	fmadd.l ip,ip,ip
 e8e:	6dbf ff87 	fmadd.l r59,r59,r59
 e92:	923f 6d87 	fmadd.l r28,r28,r28
 e96:	6dbf 6d87 	fmadd.l r27,r27,r27
 e9a:	b6bf 2487 	fmadd.l sp,sp,sp
 e9e:	983f 7a07 	fmadd.l r28,r54,r32
 ea2:	89bf 2087 	fmadd.l ip,r2,fp
 ea6:	633f 3507 	fmadd.l fp,r40,r22

00000eaa \<fmsubf16\>:
 eaa:	924f 2487 	fmsub.l ip,ip,ip
 eae:	6dc7      	fmsub r3,r3,r3
 eb0:	0047      	fmsub r0,r0,r0
 eb2:	6dcf 2487 	fmsub.l fp,fp,fp
 eb6:	b6cf 2487 	fmsub.l sp,sp,sp
 eba:	accf 2407 	fmsub.l sp,fp,r1
 ebe:	2ecf 0487 	fmsub.l r1,fp,sp
 ec2:	0c47      	fmsub r0,r3,r0

00000ec4 \<fmsubf32\>:
 ec4:	924f 2487 	fmsub.l ip,ip,ip
 ec8:	6dcf ff87 	fmsub.l r59,r59,r59
 ecc:	924f 6d87 	fmsub.l r28,r28,r28
 ed0:	6dcf 6d87 	fmsub.l r27,r27,r27
 ed4:	b6cf 2487 	fmsub.l sp,sp,sp
 ed8:	50cf a887 	fmsub.l r42,r20,sb
 edc:	c14f 4e87 	fmsub.l r22,r24,r42
 ee0:	f9cf 2907 	fmsub.l r15,r22,r19
 ee4:	2a4c 0101 	ldr.l r1,\[r2,-0xc]
 ee8:	dbbc 4dff 	strh.l r22,\[r30,-0x7ff]
 eec:	9bec 24ff 	ldrd.l ip,\[lr,\+0x7ff]
 ef0:	201e      	bitr r1,r0
 ef2:	fc1f 640e 	bitr.l r31,r15
