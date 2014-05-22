#objdump: -dr

dump\.o:     file format .*


Disassembly of section \.text:

00000000 <\.text>:
   0:	81 00                         	mov\.b	r0, 4\[r0\]
   2:	81 70                         	mov\.b	r0, 4\[r7\]
   4:	87 00                         	mov\.b	r0, 28\[r0\]
   6:	87 70                         	mov\.b	r0, 28\[r7\]
   8:	81 07                         	mov\.b	r7, 4\[r0\]
   a:	81 77                         	mov\.b	r7, 4\[r7\]
   c:	87 07                         	mov\.b	r7, 28\[r0\]
   e:	87 77                         	mov\.b	r7, 28\[r7\]
  10:	90 80                         	mov\.w	r0, 4\[r0\]
  12:	90 f0                         	mov\.w	r0, 4\[r7\]
  14:	93 80                         	mov\.w	r0, 28\[r0\]
  16:	93 f0                         	mov\.w	r0, 28\[r7\]
  18:	90 87                         	mov\.w	r7, 4\[r0\]
  1a:	90 f7                         	mov\.w	r7, 4\[r7\]
  1c:	93 87                         	mov\.w	r7, 28\[r0\]
  1e:	93 f7                         	mov\.w	r7, 28\[r7\]
  20:	a0 08                         	mov\.l	r0, 4\[r0\]
  22:	a0 78                         	mov\.l	r0, 4\[r7\]
  24:	a1 88                         	mov\.l	r0, 28\[r0\]
  26:	a1 f8                         	mov\.l	r0, 28\[r7\]
  28:	a0 0f                         	mov\.l	r7, 4\[r0\]
  2a:	a0 7f                         	mov\.l	r7, 4\[r7\]
  2c:	a1 8f                         	mov\.l	r7, 28\[r0\]
  2e:	a1 ff                         	mov\.l	r7, 28\[r7\]
  30:	89 00                         	mov\.b	4\[r0\], r0
  32:	89 07                         	mov\.b	4\[r0\], r7
  34:	89 70                         	mov\.b	4\[r7\], r0
  36:	89 77                         	mov\.b	4\[r7\], r7
  38:	8f 00                         	mov\.b	28\[r0\], r0
  3a:	8f 07                         	mov\.b	28\[r0\], r7
  3c:	8f 70                         	mov\.b	28\[r7\], r0
  3e:	8f 77                         	mov\.b	28\[r7\], r7
  40:	98 80                         	mov\.w	4\[r0\], r0
  42:	98 87                         	mov\.w	4\[r0\], r7
  44:	98 f0                         	mov\.w	4\[r7\], r0
  46:	98 f7                         	mov\.w	4\[r7\], r7
  48:	9b 80                         	mov\.w	28\[r0\], r0
  4a:	9b 87                         	mov\.w	28\[r0\], r7
  4c:	9b f0                         	mov\.w	28\[r7\], r0
  4e:	9b f7                         	mov\.w	28\[r7\], r7
  50:	a8 08                         	mov\.l	4\[r0\], r0
  52:	a8 0f                         	mov\.l	4\[r0\], r7
  54:	a8 78                         	mov\.l	4\[r7\], r0
  56:	a8 7f                         	mov\.l	4\[r7\], r7
  58:	a9 88                         	mov\.l	28\[r0\], r0
  5a:	a9 8f                         	mov\.l	28\[r0\], r7
  5c:	a9 f8                         	mov\.l	28\[r7\], r0
  5e:	a9 ff                         	mov\.l	28\[r7\], r7
  60:	66 00                         	mov\.l	#0, r0
  62:	66 0f                         	mov\.l	#0, r15
  64:	66 f0                         	mov\.l	#15, r0
  66:	66 ff                         	mov\.l	#15, r15
  68:	f9 04 04 80                   	mov\.b	#-128, 4\[r0\]
  6c:	f9 74 04 80                   	mov\.b	#-128, 4\[r7\]
  70:	f9 04 1c 80                   	mov\.b	#-128, 28\[r0\]
  74:	f9 74 1c 80                   	mov\.b	#-128, 28\[r7\]
  78:	3c 04 7f                      	mov\.b	#127, 4\[r0\]
  7b:	3c 74 7f                      	mov\.b	#127, 4\[r7\]
  7e:	3c 8c 7f                      	mov\.b	#127, 28\[r0\]
  81:	3c fc 7f                      	mov\.b	#127, 28\[r7\]
  84:	3d 02 00                      	mov\.w	#0, 4\[r0\]
  87:	3d 72 00                      	mov\.w	#0, 4\[r7\]
  8a:	3d 0e 00                      	mov\.w	#0, 28\[r0\]
  8d:	3d 7e 00                      	mov\.w	#0, 28\[r7\]
  90:	3d 02 ff                      	mov\.w	#255, 4\[r0\]
  93:	3d 72 ff                      	mov\.w	#255, 4\[r7\]
  96:	3d 0e ff                      	mov\.w	#255, 28\[r0\]
  99:	3d 7e ff                      	mov\.w	#255, 28\[r7\]
  9c:	3e 01 00                      	mov\.l	#0, 4\[r0\]
  9f:	3e 71 00                      	mov\.l	#0, 4\[r7\]
  a2:	3e 07 00                      	mov\.l	#0, 28\[r0\]
  a5:	3e 77 00                      	mov\.l	#0, 28\[r7\]
  a8:	3e 01 ff                      	mov\.l	#255, 4\[r0\]
  ab:	3e 71 ff                      	mov\.l	#255, 4\[r7\]
  ae:	3e 07 ff                      	mov\.l	#255, 28\[r0\]
  b1:	3e 77 ff                      	mov\.l	#255, 28\[r7\]
  b4:	66 00                         	mov\.l	#0, r0
  b6:	66 0f                         	mov\.l	#0, r15
  b8:	75 40 ff                      	mov\.l	#255, r0
  bb:	75 4f ff                      	mov\.l	#255, r15
  be:	fb 06 80                      	mov\.l	#-128, r0
  c1:	fb f6 80                      	mov\.l	#-128, r15
  c4:	75 40 7f                      	mov\.l	#127, r0
  c7:	75 4f 7f                      	mov\.l	#127, r15
  ca:	fb 0a 00 80                   	mov\.l	#0xffff8000, r0
  ce:	fb fa 00 80                   	mov\.l	#0xffff8000, r15
  d2:	fb 0e 00 80 00                	mov\.l	#0x8000, r0
  d7:	fb fe 00 80 00                	mov\.l	#0x8000, r15
  dc:	fb 0e 00 00 80                	mov\.l	#0xff800000, r0
  e1:	fb fe 00 00 80                	mov\.l	#0xff800000, r15
  e6:	fb 0e ff ff 7f                	mov\.l	#0x7fffff, r0
  eb:	fb fe ff ff 7f                	mov\.l	#0x7fffff, r15
  f0:	fb 02 00 00 00 80             	mov\.l	#0x80000000, r0
  f6:	fb f2 00 00 00 80             	mov\.l	#0x80000000, r15
  fc:	fb 02 ff ff ff 7f             	mov\.l	#0x7fffffff, r0
 102:	fb f2 ff ff ff 7f             	mov\.l	#0x7fffffff, r15
 108:	cf 10                         	mov\.b	r1, r0
 10a:	cf 1f                         	mov\.b	r1, r15
 10c:	cf f0                         	mov\.b	r15, r0
 10e:	cf ff                         	mov\.b	r15, r15
 110:	df 10                         	mov\.w	r1, r0
 112:	df 1f                         	mov\.w	r1, r15
 114:	df f0                         	mov\.w	r15, r0
 116:	df ff                         	mov\.w	r15, r15
 118:	ef 10                         	mov\.l	r1, r0
 11a:	ef 1f                         	mov\.l	r1, r15
 11c:	ef f0                         	mov\.l	r15, r0
 11e:	ef ff                         	mov\.l	r15, r15
 120:	3c 00 00                      	mov\.b	#0, \[r0\]
 123:	f8 f4 00                      	mov\.b	#0, \[r15\]
 126:	f9 04 fc 00                   	mov\.b	#0, 252\[r0\]
 12a:	f9 f4 fc 00                   	mov\.b	#0, 252\[r15\]
 12e:	fa 04 fc ff 00                	mov\.b	#0, 65532\[r0\]
 133:	fa f4 fc ff 00                	mov\.b	#0, 65532\[r15\]
 138:	3c 00 ff                      	mov\.b	#255, \[r0\]
 13b:	f8 f4 ff                      	mov\.b	#-1, \[r15\]
 13e:	f9 04 fc ff                   	mov\.b	#-1, 252\[r0\]
 142:	f9 f4 fc ff                   	mov\.b	#-1, 252\[r15\]
 146:	fa 04 fc ff ff                	mov\.b	#-1, 65532\[r0\]
 14b:	fa f4 fc ff ff                	mov\.b	#-1, 65532\[r15\]
 150:	f8 05 80                      	mov\.w	#-128, \[r0\]
 153:	f8 f5 80                      	mov\.w	#-128, \[r15\]
 156:	f9 05 7e 80                   	mov\.w	#-128, 252\[r0\]
 15a:	f9 f5 7e 80                   	mov\.w	#-128, 252\[r15\]
 15e:	fa 05 fe 7f 80                	mov\.w	#-128, 65532\[r0\]
 163:	fa f5 fe 7f 80                	mov\.w	#-128, 65532\[r15\]
 168:	3d 00 7f                      	mov\.w	#127, \[r0\]
 16b:	f8 f5 7f                      	mov\.w	#127, \[r15\]
 16e:	f9 05 7e 7f                   	mov\.w	#127, 252\[r0\]
 172:	f9 f5 7e 7f                   	mov\.w	#127, 252\[r15\]
 176:	fa 05 fe 7f 7f                	mov\.w	#127, 65532\[r0\]
 17b:	fa f5 fe 7f 7f                	mov\.w	#127, 65532\[r15\]
 180:	3d 00 00                      	mov\.w	#0, \[r0\]
 183:	f8 f5 00                      	mov\.w	#0, \[r15\]
 186:	f9 05 7e 00                   	mov\.w	#0, 252\[r0\]
 18a:	f9 f5 7e 00                   	mov\.w	#0, 252\[r15\]
 18e:	fa 05 fe 7f 00                	mov\.w	#0, 65532\[r0\]
 193:	fa f5 fe 7f 00                	mov\.w	#0, 65532\[r15\]
 198:	f8 09 ff ff                   	mov\.w	#-1, \[r0\]
 19c:	f8 f9 ff ff                   	mov\.w	#-1, \[r15\]
 1a0:	f9 09 7e ff ff                	mov\.w	#-1, 252\[r0\]
 1a5:	f9 f9 7e ff ff                	mov\.w	#-1, 252\[r15\]
 1aa:	fa 09 fe 7f ff ff             	mov\.w	#-1, 65532\[r0\]
 1b0:	fa f9 fe 7f ff ff             	mov\.w	#-1, 65532\[r15\]
 1b6:	f8 06 80                      	mov\.l	#-128, \[r0\]
 1b9:	f8 f6 80                      	mov\.l	#-128, \[r15\]
 1bc:	f9 06 3f 80                   	mov\.l	#-128, 252\[r0\]
 1c0:	f9 f6 3f 80                   	mov\.l	#-128, 252\[r15\]
 1c4:	fa 06 ff 3f 80                	mov\.l	#-128, 65532\[r0\]
 1c9:	fa f6 ff 3f 80                	mov\.l	#-128, 65532\[r15\]
 1ce:	3e 00 7f                      	mov\.l	#127, \[r0\]
 1d1:	f8 f6 7f                      	mov\.l	#127, \[r15\]
 1d4:	f9 06 3f 7f                   	mov\.l	#127, 252\[r0\]
 1d8:	f9 f6 3f 7f                   	mov\.l	#127, 252\[r15\]
 1dc:	fa 06 ff 3f 7f                	mov\.l	#127, 65532\[r0\]
 1e1:	fa f6 ff 3f 7f                	mov\.l	#127, 65532\[r15\]
 1e6:	f8 0a 00 80                   	mov\.l	#0xffff8000, \[r0\]
 1ea:	f8 fa 00 80                   	mov\.l	#0xffff8000, \[r15\]
 1ee:	f9 0a 3f 00 80                	mov\.l	#0xffff8000, 252\[r0\]
 1f3:	f9 fa 3f 00 80                	mov\.l	#0xffff8000, 252\[r15\]
 1f8:	fa 0a ff 3f 00 80             	mov\.l	#0xffff8000, 65532\[r0\]
 1fe:	fa fa ff 3f 00 80             	mov\.l	#0xffff8000, 65532\[r15\]
 204:	f8 0e 00 80 00                	mov\.l	#0x8000, \[r0\]
 209:	f8 fe 00 80 00                	mov\.l	#0x8000, \[r15\]
 20e:	f9 0e 3f 00 80 00             	mov\.l	#0x8000, 252\[r0\]
 214:	f9 fe 3f 00 80 00             	mov\.l	#0x8000, 252\[r15\]
 21a:	fa 0e ff 3f 00 80 00          	mov\.l	#0x8000, 65532\[r0\]
 221:	fa fe ff 3f 00 80 00          	mov\.l	#0x8000, 65532\[r15\]
 228:	f8 0e 00 00 80                	mov\.l	#0xff800000, \[r0\]
 22d:	f8 fe 00 00 80                	mov\.l	#0xff800000, \[r15\]
 232:	f9 0e 3f 00 00 80             	mov\.l	#0xff800000, 252\[r0\]
 238:	f9 fe 3f 00 00 80             	mov\.l	#0xff800000, 252\[r15\]
 23e:	fa 0e ff 3f 00 00 80          	mov\.l	#0xff800000, 65532\[r0\]
 245:	fa fe ff 3f 00 00 80          	mov\.l	#0xff800000, 65532\[r15\]
 24c:	f8 0e ff ff 7f                	mov\.l	#0x7fffff, \[r0\]
 251:	f8 fe ff ff 7f                	mov\.l	#0x7fffff, \[r15\]
 256:	f9 0e 3f ff ff 7f             	mov\.l	#0x7fffff, 252\[r0\]
 25c:	f9 fe 3f ff ff 7f             	mov\.l	#0x7fffff, 252\[r15\]
 262:	fa 0e ff 3f ff ff 7f          	mov\.l	#0x7fffff, 65532\[r0\]
 269:	fa fe ff 3f ff ff 7f          	mov\.l	#0x7fffff, 65532\[r15\]
 270:	f8 02 00 00 00 80             	mov\.l	#0x80000000, \[r0\]
 276:	f8 f2 00 00 00 80             	mov\.l	#0x80000000, \[r15\]
 27c:	f9 02 3f 00 00 00 80          	mov\.l	#0x80000000, 252\[r0\]
 283:	f9 f2 3f 00 00 00 80          	mov\.l	#0x80000000, 252\[r15\]
 28a:	fa 02 ff 3f 00 00 00 80       	mov\.l	#0x80000000, 65532\[r0\]
 292:	fa f2 ff 3f 00 00 00 80       	mov\.l	#0x80000000, 65532\[r15\]
 29a:	f8 02 ff ff ff 7f             	mov\.l	#0x7fffffff, \[r0\]
 2a0:	f8 f2 ff ff ff 7f             	mov\.l	#0x7fffffff, \[r15\]
 2a6:	f9 02 3f ff ff ff 7f          	mov\.l	#0x7fffffff, 252\[r0\]
 2ad:	f9 f2 3f ff ff ff 7f          	mov\.l	#0x7fffffff, 252\[r15\]
 2b4:	fa 02 ff 3f ff ff ff 7f       	mov\.l	#0x7fffffff, 65532\[r0\]
 2bc:	fa f2 ff 3f ff ff ff 7f       	mov\.l	#0x7fffffff, 65532\[r15\]
 2c4:	cc 00                         	mov\.b	\[r0\], r0
 2c6:	cc 0f                         	mov\.b	\[r0\], r15
 2c8:	cc f0                         	mov\.b	\[r15\], r0
 2ca:	cc ff                         	mov\.b	\[r15\], r15
 2cc:	cd 00 fc                      	mov\.b	252\[r0\], r0
 2cf:	cd 0f fc                      	mov\.b	252\[r0\], r15
 2d2:	cd f0 fc                      	mov\.b	252\[r15\], r0
 2d5:	cd ff fc                      	mov\.b	252\[r15\], r15
 2d8:	ce 00 fc ff                   	mov\.b	65532\[r0\], r0
 2dc:	ce 0f fc ff                   	mov\.b	65532\[r0\], r15
 2e0:	ce f0 fc ff                   	mov\.b	65532\[r15\], r0
 2e4:	ce ff fc ff                   	mov\.b	65532\[r15\], r15
 2e8:	dc 00                         	mov\.w	\[r0\], r0
 2ea:	dc 0f                         	mov\.w	\[r0\], r15
 2ec:	dc f0                         	mov\.w	\[r15\], r0
 2ee:	dc ff                         	mov\.w	\[r15\], r15
 2f0:	dd 00 7e                      	mov\.w	252\[r0\], r0
 2f3:	dd 0f 7e                      	mov\.w	252\[r0\], r15
 2f6:	dd f0 7e                      	mov\.w	252\[r15\], r0
 2f9:	dd ff 7e                      	mov\.w	252\[r15\], r15
 2fc:	de 00 fe 7f                   	mov\.w	65532\[r0\], r0
 300:	de 0f fe 7f                   	mov\.w	65532\[r0\], r15
 304:	de f0 fe 7f                   	mov\.w	65532\[r15\], r0
 308:	de ff fe 7f                   	mov\.w	65532\[r15\], r15
 30c:	ec 00                         	mov\.l	\[r0\], r0
 30e:	ec 0f                         	mov\.l	\[r0\], r15
 310:	ec f0                         	mov\.l	\[r15\], r0
 312:	ec ff                         	mov\.l	\[r15\], r15
 314:	ed 00 3f                      	mov\.l	252\[r0\], r0
 317:	ed 0f 3f                      	mov\.l	252\[r0\], r15
 31a:	ed f0 3f                      	mov\.l	252\[r15\], r0
 31d:	ed ff 3f                      	mov\.l	252\[r15\], r15
 320:	ee 00 ff 3f                   	mov\.l	65532\[r0\], r0
 324:	ee 0f ff 3f                   	mov\.l	65532\[r0\], r15
 328:	ee f0 ff 3f                   	mov\.l	65532\[r15\], r0
 32c:	ee ff ff 3f                   	mov\.l	65532\[r15\], r15
 330:	fe 40 00                      	mov\.b	\[r0, r0\], r0
 333:	fe 40 0f                      	mov\.b	\[r0, r0\], r15
 336:	fe 40 f0                      	mov\.b	\[r0, r15\], r0
 339:	fe 40 ff                      	mov\.b	\[r0, r15\], r15
 33c:	fe 4f 00                      	mov\.b	\[r15, r0\], r0
 33f:	fe 4f 0f                      	mov\.b	\[r15, r0\], r15
 342:	fe 4f f0                      	mov\.b	\[r15, r15\], r0
 345:	fe 4f ff                      	mov\.b	\[r15, r15\], r15
 348:	fe 50 00                      	mov\.w	\[r0, r0\], r0
 34b:	fe 50 0f                      	mov\.w	\[r0, r0\], r15
 34e:	fe 50 f0                      	mov\.w	\[r0, r15\], r0
 351:	fe 50 ff                      	mov\.w	\[r0, r15\], r15
 354:	fe 5f 00                      	mov\.w	\[r15, r0\], r0
 357:	fe 5f 0f                      	mov\.w	\[r15, r0\], r15
 35a:	fe 5f f0                      	mov\.w	\[r15, r15\], r0
 35d:	fe 5f ff                      	mov\.w	\[r15, r15\], r15
 360:	fe 60 00                      	mov\.l	\[r0, r0\], r0
 363:	fe 60 0f                      	mov\.l	\[r0, r0\], r15
 366:	fe 60 f0                      	mov\.l	\[r0, r15\], r0
 369:	fe 60 ff                      	mov\.l	\[r0, r15\], r15
 36c:	fe 6f 00                      	mov\.l	\[r15, r0\], r0
 36f:	fe 6f 0f                      	mov\.l	\[r15, r0\], r15
 372:	fe 6f f0                      	mov\.l	\[r15, r15\], r0
 375:	fe 6f ff                      	mov\.l	\[r15, r15\], r15
 378:	c3 01                         	mov\.b	r1, \[r0\]
 37a:	c3 f1                         	mov\.b	r1, \[r15\]
 37c:	c7 01 fc                      	mov\.b	r1, 252\[r0\]
 37f:	c7 f1 fc                      	mov\.b	r1, 252\[r15\]
 382:	cb 01 fc ff                   	mov\.b	r1, 65532\[r0\]
 386:	cb f1 fc ff                   	mov\.b	r1, 65532\[r15\]
 38a:	c3 0f                         	mov\.b	r15, \[r0\]
 38c:	c3 ff                         	mov\.b	r15, \[r15\]
 38e:	c7 0f fc                      	mov\.b	r15, 252\[r0\]
 391:	c7 ff fc                      	mov\.b	r15, 252\[r15\]
 394:	cb 0f fc ff                   	mov\.b	r15, 65532\[r0\]
 398:	cb ff fc ff                   	mov\.b	r15, 65532\[r15\]
 39c:	d3 01                         	mov\.w	r1, \[r0\]
 39e:	d3 f1                         	mov\.w	r1, \[r15\]
 3a0:	d7 01 7e                      	mov\.w	r1, 252\[r0\]
 3a3:	d7 f1 7e                      	mov\.w	r1, 252\[r15\]
 3a6:	db 01 fe 7f                   	mov\.w	r1, 65532\[r0\]
 3aa:	db f1 fe 7f                   	mov\.w	r1, 65532\[r15\]
 3ae:	d3 0f                         	mov\.w	r15, \[r0\]
 3b0:	d3 ff                         	mov\.w	r15, \[r15\]
 3b2:	d7 0f 7e                      	mov\.w	r15, 252\[r0\]
 3b5:	d7 ff 7e                      	mov\.w	r15, 252\[r15\]
 3b8:	db 0f fe 7f                   	mov\.w	r15, 65532\[r0\]
 3bc:	db ff fe 7f                   	mov\.w	r15, 65532\[r15\]
 3c0:	e3 01                         	mov\.l	r1, \[r0\]
 3c2:	e3 f1                         	mov\.l	r1, \[r15\]
 3c4:	e7 01 3f                      	mov\.l	r1, 252\[r0\]
 3c7:	e7 f1 3f                      	mov\.l	r1, 252\[r15\]
 3ca:	eb 01 ff 3f                   	mov\.l	r1, 65532\[r0\]
 3ce:	eb f1 ff 3f                   	mov\.l	r1, 65532\[r15\]
 3d2:	e3 0f                         	mov\.l	r15, \[r0\]
 3d4:	e3 ff                         	mov\.l	r15, \[r15\]
 3d6:	e7 0f 3f                      	mov\.l	r15, 252\[r0\]
 3d9:	e7 ff 3f                      	mov\.l	r15, 252\[r15\]
 3dc:	eb 0f ff 3f                   	mov\.l	r15, 65532\[r0\]
 3e0:	eb ff ff 3f                   	mov\.l	r15, 65532\[r15\]
 3e4:	fe 00 00                      	mov\.b	r0, \[r0, r0\]
 3e7:	fe 00 f0                      	mov\.b	r0, \[r0, r15\]
 3ea:	fe 0f 00                      	mov\.b	r0, \[r15, r0\]
 3ed:	fe 0f f0                      	mov\.b	r0, \[r15, r15\]
 3f0:	fe 00 0f                      	mov\.b	r15, \[r0, r0\]
 3f3:	fe 00 ff                      	mov\.b	r15, \[r0, r15\]
 3f6:	fe 0f 0f                      	mov\.b	r15, \[r15, r0\]
 3f9:	fe 0f ff                      	mov\.b	r15, \[r15, r15\]
 3fc:	fe 10 00                      	mov\.w	r0, \[r0, r0\]
 3ff:	fe 10 f0                      	mov\.w	r0, \[r0, r15\]
 402:	fe 1f 00                      	mov\.w	r0, \[r15, r0\]
 405:	fe 1f f0                      	mov\.w	r0, \[r15, r15\]
 408:	fe 10 0f                      	mov\.w	r15, \[r0, r0\]
 40b:	fe 10 ff                      	mov\.w	r15, \[r0, r15\]
 40e:	fe 1f 0f                      	mov\.w	r15, \[r15, r0\]
 411:	fe 1f ff                      	mov\.w	r15, \[r15, r15\]
 414:	fe 20 00                      	mov\.l	r0, \[r0, r0\]
 417:	fe 20 f0                      	mov\.l	r0, \[r0, r15\]
 41a:	fe 2f 00                      	mov\.l	r0, \[r15, r0\]
 41d:	fe 2f f0                      	mov\.l	r0, \[r15, r15\]
 420:	fe 20 0f                      	mov\.l	r15, \[r0, r0\]
 423:	fe 20 ff                      	mov\.l	r15, \[r0, r15\]
 426:	fe 2f 0f                      	mov\.l	r15, \[r15, r0\]
 429:	fe 2f ff                      	mov\.l	r15, \[r15, r15\]
 42c:	c0 00                         	mov\.b	\[r0\], \[r0\]
 42e:	c0 0f                         	mov\.b	\[r0\], \[r15\]
 430:	c4 00 fc                      	mov\.b	\[r0\], 252\[r0\]
 433:	c4 0f fc                      	mov\.b	\[r0\], 252\[r15\]
 436:	c8 00 fc ff                   	mov\.b	\[r0\], 65532\[r0\]
 43a:	c8 0f fc ff                   	mov\.b	\[r0\], 65532\[r15\]
 43e:	c0 f0                         	mov\.b	\[r15\], \[r0\]
 440:	c0 ff                         	mov\.b	\[r15\], \[r15\]
 442:	c4 f0 fc                      	mov\.b	\[r15\], 252\[r0\]
 445:	c4 ff fc                      	mov\.b	\[r15\], 252\[r15\]
 448:	c8 f0 fc ff                   	mov\.b	\[r15\], 65532\[r0\]
 44c:	c8 ff fc ff                   	mov\.b	\[r15\], 65532\[r15\]
 450:	c1 00 fc                      	mov\.b	252\[r0\], \[r0\]
 453:	c1 0f fc                      	mov\.b	252\[r0\], \[r15\]
 456:	c5 00 fc fc                   	mov\.b	252\[r0\], 252\[r0\]
 45a:	c5 0f fc fc                   	mov\.b	252\[r0\], 252\[r15\]
 45e:	c9 00 fc fc ff                	mov\.b	252\[r0\], 65532\[r0\]
 463:	c9 0f fc fc ff                	mov\.b	252\[r0\], 65532\[r15\]
 468:	c1 f0 fc                      	mov\.b	252\[r15\], \[r0\]
 46b:	c1 ff fc                      	mov\.b	252\[r15\], \[r15\]
 46e:	c5 f0 fc fc                   	mov\.b	252\[r15\], 252\[r0\]
 472:	c5 ff fc fc                   	mov\.b	252\[r15\], 252\[r15\]
 476:	c9 f0 fc fc ff                	mov\.b	252\[r15\], 65532\[r0\]
 47b:	c9 ff fc fc ff                	mov\.b	252\[r15\], 65532\[r15\]
 480:	c2 00 fc ff                   	mov\.b	65532\[r0\], \[r0\]
 484:	c2 0f fc ff                   	mov\.b	65532\[r0\], \[r15\]
 488:	c6 00 fc ff fc                	mov\.b	65532\[r0\], 252\[r0\]
 48d:	c6 0f fc ff fc                	mov\.b	65532\[r0\], 252\[r15\]
 492:	ca 00 fc ff fc ff             	mov\.b	65532\[r0\], 65532\[r0\]
 498:	ca 0f fc ff fc ff             	mov\.b	65532\[r0\], 65532\[r15\]
 49e:	c2 f0 fc ff                   	mov\.b	65532\[r15\], \[r0\]
 4a2:	c2 ff fc ff                   	mov\.b	65532\[r15\], \[r15\]
 4a6:	c6 f0 fc ff fc                	mov\.b	65532\[r15\], 252\[r0\]
 4ab:	c6 ff fc ff fc                	mov\.b	65532\[r15\], 252\[r15\]
 4b0:	ca f0 fc ff fc ff             	mov\.b	65532\[r15\], 65532\[r0\]
 4b6:	ca ff fc ff fc ff             	mov\.b	65532\[r15\], 65532\[r15\]
 4bc:	d0 00                         	mov\.w	\[r0\], \[r0\]
 4be:	d0 0f                         	mov\.w	\[r0\], \[r15\]
 4c0:	d4 00 7e                      	mov\.w	\[r0\], 252\[r0\]
 4c3:	d4 0f 7e                      	mov\.w	\[r0\], 252\[r15\]
 4c6:	d8 00 fe 7f                   	mov\.w	\[r0\], 65532\[r0\]
 4ca:	d8 0f fe 7f                   	mov\.w	\[r0\], 65532\[r15\]
 4ce:	d0 f0                         	mov\.w	\[r15\], \[r0\]
 4d0:	d0 ff                         	mov\.w	\[r15\], \[r15\]
 4d2:	d4 f0 7e                      	mov\.w	\[r15\], 252\[r0\]
 4d5:	d4 ff 7e                      	mov\.w	\[r15\], 252\[r15\]
 4d8:	d8 f0 fe 7f                   	mov\.w	\[r15\], 65532\[r0\]
 4dc:	d8 ff fe 7f                   	mov\.w	\[r15\], 65532\[r15\]
 4e0:	d1 00 7e                      	mov\.w	252\[r0\], \[r0\]
 4e3:	d1 0f 7e                      	mov\.w	252\[r0\], \[r15\]
 4e6:	d5 00 7e 7e                   	mov\.w	252\[r0\], 252\[r0\]
 4ea:	d5 0f 7e 7e                   	mov\.w	252\[r0\], 252\[r15\]
 4ee:	d9 00 7e fe 7f                	mov\.w	252\[r0\], 65532\[r0\]
 4f3:	d9 0f 7e fe 7f                	mov\.w	252\[r0\], 65532\[r15\]
 4f8:	d1 f0 7e                      	mov\.w	252\[r15\], \[r0\]
 4fb:	d1 ff 7e                      	mov\.w	252\[r15\], \[r15\]
 4fe:	d5 f0 7e 7e                   	mov\.w	252\[r15\], 252\[r0\]
 502:	d5 ff 7e 7e                   	mov\.w	252\[r15\], 252\[r15\]
 506:	d9 f0 7e fe 7f                	mov\.w	252\[r15\], 65532\[r0\]
 50b:	d9 ff 7e fe 7f                	mov\.w	252\[r15\], 65532\[r15\]
 510:	d2 00 fe 7f                   	mov\.w	65532\[r0\], \[r0\]
 514:	d2 0f fe 7f                   	mov\.w	65532\[r0\], \[r15\]
 518:	d6 00 fe 7f 7e                	mov\.w	65532\[r0\], 252\[r0\]
 51d:	d6 0f fe 7f 7e                	mov\.w	65532\[r0\], 252\[r15\]
 522:	da 00 fe 7f fe 7f             	mov\.w	65532\[r0\], 65532\[r0\]
 528:	da 0f fe 7f fe 7f             	mov\.w	65532\[r0\], 65532\[r15\]
 52e:	d2 f0 fe 7f                   	mov\.w	65532\[r15\], \[r0\]
 532:	d2 ff fe 7f                   	mov\.w	65532\[r15\], \[r15\]
 536:	d6 f0 fe 7f 7e                	mov\.w	65532\[r15\], 252\[r0\]
 53b:	d6 ff fe 7f 7e                	mov\.w	65532\[r15\], 252\[r15\]
 540:	da f0 fe 7f fe 7f             	mov\.w	65532\[r15\], 65532\[r0\]
 546:	da ff fe 7f fe 7f             	mov\.w	65532\[r15\], 65532\[r15\]
 54c:	e0 00                         	mov\.l	\[r0\], \[r0\]
 54e:	e0 0f                         	mov\.l	\[r0\], \[r15\]
 550:	e4 00 3f                      	mov\.l	\[r0\], 252\[r0\]
 553:	e4 0f 3f                      	mov\.l	\[r0\], 252\[r15\]
 556:	e8 00 ff 3f                   	mov\.l	\[r0\], 65532\[r0\]
 55a:	e8 0f ff 3f                   	mov\.l	\[r0\], 65532\[r15\]
 55e:	e0 f0                         	mov\.l	\[r15\], \[r0\]
 560:	e0 ff                         	mov\.l	\[r15\], \[r15\]
 562:	e4 f0 3f                      	mov\.l	\[r15\], 252\[r0\]
 565:	e4 ff 3f                      	mov\.l	\[r15\], 252\[r15\]
 568:	e8 f0 ff 3f                   	mov\.l	\[r15\], 65532\[r0\]
 56c:	e8 ff ff 3f                   	mov\.l	\[r15\], 65532\[r15\]
 570:	e1 00 3f                      	mov\.l	252\[r0\], \[r0\]
 573:	e1 0f 3f                      	mov\.l	252\[r0\], \[r15\]
 576:	e5 00 3f 3f                   	mov\.l	252\[r0\], 252\[r0\]
 57a:	e5 0f 3f 3f                   	mov\.l	252\[r0\], 252\[r15\]
 57e:	e9 00 3f ff 3f                	mov\.l	252\[r0\], 65532\[r0\]
 583:	e9 0f 3f ff 3f                	mov\.l	252\[r0\], 65532\[r15\]
 588:	e1 f0 3f                      	mov\.l	252\[r15\], \[r0\]
 58b:	e1 ff 3f                      	mov\.l	252\[r15\], \[r15\]
 58e:	e5 f0 3f 3f                   	mov\.l	252\[r15\], 252\[r0\]
 592:	e5 ff 3f 3f                   	mov\.l	252\[r15\], 252\[r15\]
 596:	e9 f0 3f ff 3f                	mov\.l	252\[r15\], 65532\[r0\]
 59b:	e9 ff 3f ff 3f                	mov\.l	252\[r15\], 65532\[r15\]
 5a0:	e2 00 ff 3f                   	mov\.l	65532\[r0\], \[r0\]
 5a4:	e2 0f ff 3f                   	mov\.l	65532\[r0\], \[r15\]
 5a8:	e6 00 ff 3f 3f                	mov\.l	65532\[r0\], 252\[r0\]
 5ad:	e6 0f ff 3f 3f                	mov\.l	65532\[r0\], 252\[r15\]
 5b2:	ea 00 ff 3f ff 3f             	mov\.l	65532\[r0\], 65532\[r0\]
 5b8:	ea 0f ff 3f ff 3f             	mov\.l	65532\[r0\], 65532\[r15\]
 5be:	e2 f0 ff 3f                   	mov\.l	65532\[r15\], \[r0\]
 5c2:	e2 ff ff 3f                   	mov\.l	65532\[r15\], \[r15\]
 5c6:	e6 f0 ff 3f 3f                	mov\.l	65532\[r15\], 252\[r0\]
 5cb:	e6 ff ff 3f 3f                	mov\.l	65532\[r15\], 252\[r15\]
 5d0:	ea f0 ff 3f ff 3f             	mov\.l	65532\[r15\], 65532\[r0\]
 5d6:	ea ff ff 3f ff 3f             	mov\.l	65532\[r15\], 65532\[r15\]
 5dc:	fd 20 00                      	mov\.b	r0, \[r0\+\]
 5df:	fd 20 f0                      	mov\.b	r0, \[r15\+\]
 5e2:	fd 20 0f                      	mov\.b	r15, \[r0\+\]
 5e5:	fd 20 ff                      	mov\.b	r15, \[r15\+\]
 5e8:	fd 21 00                      	mov\.w	r0, \[r0\+\]
 5eb:	fd 21 f0                      	mov\.w	r0, \[r15\+\]
 5ee:	fd 21 0f                      	mov\.w	r15, \[r0\+\]
 5f1:	fd 21 ff                      	mov\.w	r15, \[r15\+\]
 5f4:	fd 22 00                      	mov\.l	r0, \[r0\+\]
 5f7:	fd 22 f0                      	mov\.l	r0, \[r15\+\]
 5fa:	fd 22 0f                      	mov\.l	r15, \[r0\+\]
 5fd:	fd 22 ff                      	mov\.l	r15, \[r15\+\]
 600:	fd 28 00                      	mov\.b	\[r0\+\], r0
 603:	fd 28 0f                      	mov\.b	\[r0\+\], r15
 606:	fd 28 f0                      	mov\.b	\[r15\+\], r0
 609:	fd 28 ff                      	mov\.b	\[r15\+\], r15
 60c:	fd 29 00                      	mov\.w	\[r0\+\], r0
 60f:	fd 29 0f                      	mov\.w	\[r0\+\], r15
 612:	fd 29 f0                      	mov\.w	\[r15\+\], r0
 615:	fd 29 ff                      	mov\.w	\[r15\+\], r15
 618:	fd 2a 00                      	mov\.l	\[r0\+\], r0
 61b:	fd 2a 0f                      	mov\.l	\[r0\+\], r15
 61e:	fd 2a f0                      	mov\.l	\[r15\+\], r0
 621:	fd 2a ff                      	mov\.l	\[r15\+\], r15
 624:	fd 24 00                      	mov\.b	r0, \[-r0\]
 627:	fd 24 f0                      	mov\.b	r0, \[-r15\]
 62a:	fd 24 0f                      	mov\.b	r15, \[-r0\]
 62d:	fd 24 ff                      	mov\.b	r15, \[-r15\]
 630:	fd 25 00                      	mov\.w	r0, \[-r0\]
 633:	fd 25 f0                      	mov\.w	r0, \[-r15\]
 636:	fd 25 0f                      	mov\.w	r15, \[-r0\]
 639:	fd 25 ff                      	mov\.w	r15, \[-r15\]
 63c:	fd 26 00                      	mov\.l	r0, \[-r0\]
 63f:	fd 26 f0                      	mov\.l	r0, \[-r15\]
 642:	fd 26 0f                      	mov\.l	r15, \[-r0\]
 645:	fd 26 ff                      	mov\.l	r15, \[-r15\]
 648:	fd 2c 00                      	mov\.b	\[-r0\], r0
 64b:	fd 2c 0f                      	mov\.b	\[-r0\], r15
 64e:	fd 2c f0                      	mov\.b	\[-r15\], r0
 651:	fd 2c ff                      	mov\.b	\[-r15\], r15
 654:	fd 2d 00                      	mov\.w	\[-r0\], r0
 657:	fd 2d 0f                      	mov\.w	\[-r0\], r15
 65a:	fd 2d f0                      	mov\.w	\[-r15\], r0
 65d:	fd 2d ff                      	mov\.w	\[-r15\], r15
 660:	fd 2e 00                      	mov\.l	\[-r0\], r0
 663:	fd 2e 0f                      	mov\.l	\[-r0\], r15
 666:	fd 2e f0                      	mov\.l	\[-r15\], r0
 669:	fd 2e ff                      	mov\.l	\[-r15\], r15
