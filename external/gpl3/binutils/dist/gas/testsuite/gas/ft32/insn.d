#as:
#objdump: -d
#name: insn

dump.o:     file format elf32-ft32


Disassembly of section .text:

00000000 <pmlabel>:
   0:	00 00 f0 45 	45f00000 add.l \$r31,\$r0,\$r0
   4:	00 80 0f 44 	440f8000 add.l \$r0,\$r31,\$r0
   8:	f0 01 00 44 	440001f0 add.l \$r0,\$r0,\$r31
   c:	40 00 11 44 	44110040 add.l \$r1,\$r2,\$r4
  10:	00 00 88 44 	44880000 add.l \$r8,\$r16,\$r0
  14:	00 60 f0 45 	45f06000 add.l \$r31,\$r0,0 <pmlabel>
  18:	00 c0 0f 44 	440fc000 move.l \$r0,\$r31
  1c:	10 c0 0f 44 	440fc010 add.l \$r0,\$r31,1 <pmlabel\+0x1>
  20:	f0 df 0f 44 	440fdff0 add.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
  24:	10 80 0f 42 	420f8010 add.s \$r0,\$r31,\$r1
  28:	d0 c4 0f 42 	420fc4d0 add.s \$r0,\$r31,4d <pmlabel\+0x4d>
  2c:	10 80 0f 40 	400f8010 add.b \$r0,\$r31,\$r1
  30:	d0 c4 0f 40 	400fc4d0 add.b \$r0,\$r31,4d <pmlabel\+0x4d>
  34:	02 00 f0 45 	45f00002 sub.l \$r31,\$r0,\$r0
  38:	02 80 0f 44 	440f8002 sub.l \$r0,\$r31,\$r0
  3c:	f2 01 00 44 	440001f2 sub.l \$r0,\$r0,\$r31
  40:	42 00 11 44 	44110042 sub.l \$r1,\$r2,\$r4
  44:	02 00 88 44 	44880002 sub.l \$r8,\$r16,\$r0
  48:	02 60 f0 45 	45f06002 sub.l \$r31,\$r0,0 <pmlabel>
  4c:	02 c0 0f 44 	440fc002 sub.l \$r0,\$r31,0 <pmlabel>
  50:	12 c0 0f 44 	440fc012 sub.l \$r0,\$r31,1 <pmlabel\+0x1>
  54:	f2 df 0f 44 	440fdff2 sub.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
  58:	12 80 0f 42 	420f8012 sub.s \$r0,\$r31,\$r1
  5c:	d2 c4 0f 42 	420fc4d2 sub.s \$r0,\$r31,4d <pmlabel\+0x4d>
  60:	12 80 0f 40 	400f8012 sub.b \$r0,\$r31,\$r1
  64:	d2 c4 0f 40 	400fc4d2 sub.b \$r0,\$r31,4d <pmlabel\+0x4d>
  68:	04 00 f0 45 	45f00004 and.l \$r31,\$r0,\$r0
  6c:	04 80 0f 44 	440f8004 and.l \$r0,\$r31,\$r0
  70:	f4 01 00 44 	440001f4 and.l \$r0,\$r0,\$r31
  74:	44 00 11 44 	44110044 and.l \$r1,\$r2,\$r4
  78:	04 00 88 44 	44880004 and.l \$r8,\$r16,\$r0
  7c:	04 60 f0 45 	45f06004 and.l \$r31,\$r0,0 <pmlabel>
  80:	04 c0 0f 44 	440fc004 and.l \$r0,\$r31,0 <pmlabel>
  84:	14 c0 0f 44 	440fc014 and.l \$r0,\$r31,1 <pmlabel\+0x1>
  88:	f4 df 0f 44 	440fdff4 and.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
  8c:	14 80 0f 42 	420f8014 and.s \$r0,\$r31,\$r1
  90:	d4 c4 0f 42 	420fc4d4 and.s \$r0,\$r31,4d <pmlabel\+0x4d>
  94:	14 80 0f 40 	400f8014 and.b \$r0,\$r31,\$r1
  98:	d4 c4 0f 40 	400fc4d4 and.b \$r0,\$r31,4d <pmlabel\+0x4d>
  9c:	05 00 f0 45 	45f00005 or.l \$r31,\$r0,\$r0
  a0:	05 80 0f 44 	440f8005 or.l \$r0,\$r31,\$r0
  a4:	f5 01 00 44 	440001f5 or.l \$r0,\$r0,\$r31
  a8:	45 00 11 44 	44110045 or.l \$r1,\$r2,\$r4
  ac:	05 00 88 44 	44880005 or.l \$r8,\$r16,\$r0
  b0:	05 60 f0 45 	45f06005 or.l \$r31,\$r0,0 <pmlabel>
  b4:	05 c0 0f 44 	440fc005 or.l \$r0,\$r31,0 <pmlabel>
  b8:	15 c0 0f 44 	440fc015 or.l \$r0,\$r31,1 <pmlabel\+0x1>
  bc:	f5 df 0f 44 	440fdff5 or.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
  c0:	15 80 0f 42 	420f8015 or.s \$r0,\$r31,\$r1
  c4:	d5 c4 0f 42 	420fc4d5 or.s \$r0,\$r31,4d <pmlabel\+0x4d>
  c8:	15 80 0f 40 	400f8015 or.b \$r0,\$r31,\$r1
  cc:	d5 c4 0f 40 	400fc4d5 or.b \$r0,\$r31,4d <pmlabel\+0x4d>
  d0:	06 00 f0 45 	45f00006 xor.l \$r31,\$r0,\$r0
  d4:	06 80 0f 44 	440f8006 xor.l \$r0,\$r31,\$r0
  d8:	f6 01 00 44 	440001f6 xor.l \$r0,\$r0,\$r31
  dc:	46 00 11 44 	44110046 xor.l \$r1,\$r2,\$r4
  e0:	06 00 88 44 	44880006 xor.l \$r8,\$r16,\$r0
  e4:	06 60 f0 45 	45f06006 xor.l \$r31,\$r0,0 <pmlabel>
  e8:	06 c0 0f 44 	440fc006 xor.l \$r0,\$r31,0 <pmlabel>
  ec:	16 c0 0f 44 	440fc016 xor.l \$r0,\$r31,1 <pmlabel\+0x1>
  f0:	f6 df 0f 44 	440fdff6 xor.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
  f4:	16 80 0f 42 	420f8016 xor.s \$r0,\$r31,\$r1
  f8:	d6 c4 0f 42 	420fc4d6 xor.s \$r0,\$r31,4d <pmlabel\+0x4d>
  fc:	16 80 0f 40 	400f8016 xor.b \$r0,\$r31,\$r1
 100:	d6 c4 0f 40 	400fc4d6 xor.b \$r0,\$r31,4d <pmlabel\+0x4d>
 104:	07 00 f0 45 	45f00007 xnor.l \$r31,\$r0,\$r0
 108:	07 80 0f 44 	440f8007 xnor.l \$r0,\$r31,\$r0
 10c:	f7 01 00 44 	440001f7 xnor.l \$r0,\$r0,\$r31
 110:	47 00 11 44 	44110047 xnor.l \$r1,\$r2,\$r4
 114:	07 00 88 44 	44880007 xnor.l \$r8,\$r16,\$r0
 118:	07 60 f0 45 	45f06007 xnor.l \$r31,\$r0,0 <pmlabel>
 11c:	07 c0 0f 44 	440fc007 xnor.l \$r0,\$r31,0 <pmlabel>
 120:	17 c0 0f 44 	440fc017 xnor.l \$r0,\$r31,1 <pmlabel\+0x1>
 124:	f7 df 0f 44 	440fdff7 xnor.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 128:	17 80 0f 42 	420f8017 xnor.s \$r0,\$r31,\$r1
 12c:	d7 c4 0f 42 	420fc4d7 xnor.s \$r0,\$r31,4d <pmlabel\+0x4d>
 130:	17 80 0f 40 	400f8017 xnor.b \$r0,\$r31,\$r1
 134:	d7 c4 0f 40 	400fc4d7 xnor.b \$r0,\$r31,4d <pmlabel\+0x4d>
 138:	08 00 f0 45 	45f00008 ashl.l \$r31,\$r0,\$r0
 13c:	08 80 0f 44 	440f8008 ashl.l \$r0,\$r31,\$r0
 140:	f8 01 00 44 	440001f8 ashl.l \$r0,\$r0,\$r31
 144:	48 00 11 44 	44110048 ashl.l \$r1,\$r2,\$r4
 148:	08 00 88 44 	44880008 ashl.l \$r8,\$r16,\$r0
 14c:	08 60 f0 45 	45f06008 ashl.l \$r31,\$r0,0 <pmlabel>
 150:	08 c0 0f 44 	440fc008 ashl.l \$r0,\$r31,0 <pmlabel>
 154:	18 c0 0f 44 	440fc018 ashl.l \$r0,\$r31,1 <pmlabel\+0x1>
 158:	f8 df 0f 44 	440fdff8 ashl.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 15c:	18 80 0f 42 	420f8018 ashl.s \$r0,\$r31,\$r1
 160:	d8 c4 0f 42 	420fc4d8 ashl.s \$r0,\$r31,4d <pmlabel\+0x4d>
 164:	18 80 0f 40 	400f8018 ashl.b \$r0,\$r31,\$r1
 168:	d8 c4 0f 40 	400fc4d8 ashl.b \$r0,\$r31,4d <pmlabel\+0x4d>
 16c:	09 00 f0 45 	45f00009 lshr.l \$r31,\$r0,\$r0
 170:	09 80 0f 44 	440f8009 lshr.l \$r0,\$r31,\$r0
 174:	f9 01 00 44 	440001f9 lshr.l \$r0,\$r0,\$r31
 178:	49 00 11 44 	44110049 lshr.l \$r1,\$r2,\$r4
 17c:	09 00 88 44 	44880009 lshr.l \$r8,\$r16,\$r0
 180:	09 60 f0 45 	45f06009 lshr.l \$r31,\$r0,0 <pmlabel>
 184:	09 c0 0f 44 	440fc009 lshr.l \$r0,\$r31,0 <pmlabel>
 188:	19 c0 0f 44 	440fc019 lshr.l \$r0,\$r31,1 <pmlabel\+0x1>
 18c:	f9 df 0f 44 	440fdff9 lshr.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 190:	19 80 0f 42 	420f8019 lshr.s \$r0,\$r31,\$r1
 194:	d9 c4 0f 42 	420fc4d9 lshr.s \$r0,\$r31,4d <pmlabel\+0x4d>
 198:	19 80 0f 40 	400f8019 lshr.b \$r0,\$r31,\$r1
 19c:	d9 c4 0f 40 	400fc4d9 lshr.b \$r0,\$r31,4d <pmlabel\+0x4d>
 1a0:	0a 00 f0 45 	45f0000a ashr.l \$r31,\$r0,\$r0
 1a4:	0a 80 0f 44 	440f800a ashr.l \$r0,\$r31,\$r0
 1a8:	fa 01 00 44 	440001fa ashr.l \$r0,\$r0,\$r31
 1ac:	4a 00 11 44 	4411004a ashr.l \$r1,\$r2,\$r4
 1b0:	0a 00 88 44 	4488000a ashr.l \$r8,\$r16,\$r0
 1b4:	0a 60 f0 45 	45f0600a ashr.l \$r31,\$r0,0 <pmlabel>
 1b8:	0a c0 0f 44 	440fc00a ashr.l \$r0,\$r31,0 <pmlabel>
 1bc:	1a c0 0f 44 	440fc01a ashr.l \$r0,\$r31,1 <pmlabel\+0x1>
 1c0:	fa df 0f 44 	440fdffa ashr.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 1c4:	1a 80 0f 42 	420f801a ashr.s \$r0,\$r31,\$r1
 1c8:	da c4 0f 42 	420fc4da ashr.s \$r0,\$r31,4d <pmlabel\+0x4d>
 1cc:	1a 80 0f 40 	400f801a ashr.b \$r0,\$r31,\$r1
 1d0:	da c4 0f 40 	400fc4da ashr.b \$r0,\$r31,4d <pmlabel\+0x4d>
 1d4:	01 00 f0 45 	45f00001 ror.l \$r31,\$r0,\$r0
 1d8:	01 80 0f 44 	440f8001 ror.l \$r0,\$r31,\$r0
 1dc:	f1 01 00 44 	440001f1 ror.l \$r0,\$r0,\$r31
 1e0:	41 00 11 44 	44110041 ror.l \$r1,\$r2,\$r4
 1e4:	01 00 88 44 	44880001 ror.l \$r8,\$r16,\$r0
 1e8:	01 60 f0 45 	45f06001 ror.l \$r31,\$r0,0 <pmlabel>
 1ec:	01 c0 0f 44 	440fc001 ror.l \$r0,\$r31,0 <pmlabel>
 1f0:	11 c0 0f 44 	440fc011 ror.l \$r0,\$r31,1 <pmlabel\+0x1>
 1f4:	f1 df 0f 44 	440fdff1 ror.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 1f8:	11 80 0f 42 	420f8011 ror.s \$r0,\$r31,\$r1
 1fc:	d1 c4 0f 42 	420fc4d1 ror.s \$r0,\$r31,4d <pmlabel\+0x4d>
 200:	11 80 0f 40 	400f8011 ror.b \$r0,\$r31,\$r1
 204:	d1 c4 0f 40 	400fc4d1 ror.b \$r0,\$r31,4d <pmlabel\+0x4d>
 208:	03 00 f0 45 	45f00003 ldl.l \$r31,\$r0,\$r0
 20c:	03 80 0f 44 	440f8003 ldl.l \$r0,\$r31,\$r0
 210:	f3 01 00 44 	440001f3 ldl.l \$r0,\$r0,\$r31
 214:	43 00 11 44 	44110043 ldl.l \$r1,\$r2,\$r4
 218:	03 00 88 44 	44880003 ldl.l \$r8,\$r16,\$r0
 21c:	03 60 f0 45 	45f06003 ldl.l \$r31,\$r0,0 <pmlabel>
 220:	03 c0 0f 44 	440fc003 ldl.l \$r0,\$r31,0 <pmlabel>
 224:	13 c0 0f 44 	440fc013 ldl.l \$r0,\$r31,1 <pmlabel\+0x1>
 228:	f3 df 0f 44 	440fdff3 ldl.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 22c:	13 80 0f 42 	420f8013 ldl.s \$r0,\$r31,\$r1
 230:	d3 c4 0f 42 	420fc4d3 ldl.s \$r0,\$r31,4d <pmlabel\+0x4d>
 234:	13 80 0f 40 	400f8013 ldl.b \$r0,\$r31,\$r1
 238:	d3 c4 0f 40 	400fc4d3 ldl.b \$r0,\$r31,4d <pmlabel\+0x4d>
 23c:	0b 00 f0 45 	45f0000b bins.l \$r31,\$r0,\$r0
 240:	0b 80 0f 44 	440f800b bins.l \$r0,\$r31,\$r0
 244:	fb 01 00 44 	440001fb bins.l \$r0,\$r0,\$r31
 248:	4b 00 11 44 	4411004b bins.l \$r1,\$r2,\$r4
 24c:	0b 00 88 44 	4488000b bins.l \$r8,\$r16,\$r0
 250:	0b 60 f0 45 	45f0600b bins.l \$r31,\$r0,0 <pmlabel>
 254:	0b c0 0f 44 	440fc00b bins.l \$r0,\$r31,0 <pmlabel>
 258:	1b c0 0f 44 	440fc01b bins.l \$r0,\$r31,1 <pmlabel\+0x1>
 25c:	fb df 0f 44 	440fdffb bins.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 260:	1b 80 0f 42 	420f801b bins.s \$r0,\$r31,\$r1
 264:	db c4 0f 42 	420fc4db bins.s \$r0,\$r31,4d <pmlabel\+0x4d>
 268:	1b 80 0f 40 	400f801b bins.b \$r0,\$r31,\$r1
 26c:	db c4 0f 40 	400fc4db bins.b \$r0,\$r31,4d <pmlabel\+0x4d>
 270:	0c 00 f0 45 	45f0000c bexts.l \$r31,\$r0,\$r0
 274:	0c 80 0f 44 	440f800c bexts.l \$r0,\$r31,\$r0
 278:	fc 01 00 44 	440001fc bexts.l \$r0,\$r0,\$r31
 27c:	4c 00 11 44 	4411004c bexts.l \$r1,\$r2,\$r4
 280:	0c 00 88 44 	4488000c bexts.l \$r8,\$r16,\$r0
 284:	0c 60 f0 45 	45f0600c bexts.l \$r31,\$r0,0 <pmlabel>
 288:	0c c0 0f 44 	440fc00c bexts.l \$r0,\$r31,0 <pmlabel>
 28c:	1c c0 0f 44 	440fc01c bexts.l \$r0,\$r31,1 <pmlabel\+0x1>
 290:	fc df 0f 44 	440fdffc bexts.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 294:	1c 80 0f 42 	420f801c bexts.s \$r0,\$r31,\$r1
 298:	dc c4 0f 42 	420fc4dc bexts.s \$r0,\$r31,4d <pmlabel\+0x4d>
 29c:	1c 80 0f 40 	400f801c bexts.b \$r0,\$r31,\$r1
 2a0:	dc c4 0f 40 	400fc4dc bexts.b \$r0,\$r31,4d <pmlabel\+0x4d>
 2a4:	0d 00 f0 45 	45f0000d bextu.l \$r31,\$r0,\$r0
 2a8:	0d 80 0f 44 	440f800d bextu.l \$r0,\$r31,\$r0
 2ac:	fd 01 00 44 	440001fd bextu.l \$r0,\$r0,\$r31
 2b0:	4d 00 11 44 	4411004d bextu.l \$r1,\$r2,\$r4
 2b4:	0d 00 88 44 	4488000d bextu.l \$r8,\$r16,\$r0
 2b8:	0d 60 f0 45 	45f0600d bextu.l \$r31,\$r0,0 <pmlabel>
 2bc:	0d c0 0f 44 	440fc00d bextu.l \$r0,\$r31,0 <pmlabel>
 2c0:	1d c0 0f 44 	440fc01d bextu.l \$r0,\$r31,1 <pmlabel\+0x1>
 2c4:	fd df 0f 44 	440fdffd bextu.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 2c8:	1d 80 0f 42 	420f801d bextu.s \$r0,\$r31,\$r1
 2cc:	dd c4 0f 42 	420fc4dd bextu.s \$r0,\$r31,4d <pmlabel\+0x4d>
 2d0:	1d 80 0f 40 	400f801d bextu.b \$r0,\$r31,\$r1
 2d4:	dd c4 0f 40 	400fc4dd bextu.b \$r0,\$r31,4d <pmlabel\+0x4d>
 2d8:	0e 00 f0 45 	45f0000e flip.l \$r31,\$r0,\$r0
 2dc:	0e 80 0f 44 	440f800e flip.l \$r0,\$r31,\$r0
 2e0:	fe 01 00 44 	440001fe flip.l \$r0,\$r0,\$r31
 2e4:	4e 00 11 44 	4411004e flip.l \$r1,\$r2,\$r4
 2e8:	0e 00 88 44 	4488000e flip.l \$r8,\$r16,\$r0
 2ec:	0e 60 f0 45 	45f0600e flip.l \$r31,\$r0,0 <pmlabel>
 2f0:	0e c0 0f 44 	440fc00e flip.l \$r0,\$r31,0 <pmlabel>
 2f4:	1e c0 0f 44 	440fc01e flip.l \$r0,\$r31,1 <pmlabel\+0x1>
 2f8:	fe df 0f 44 	440fdffe flip.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 2fc:	1e 80 0f 42 	420f801e flip.s \$r0,\$r31,\$r1
 300:	de c4 0f 42 	420fc4de flip.s \$r0,\$r31,4d <pmlabel\+0x4d>
 304:	1e 80 0f 40 	400f801e flip.b \$r0,\$r31,\$r1
 308:	de c4 0f 40 	400fc4de flip.b \$r0,\$r31,4d <pmlabel\+0x4d>
 30c:	00 00 e0 5d 	5de00000 addcc.l \$r0,\$r0
 310:	00 80 ef 5d 	5def8000 addcc.l \$r31,\$r0
 314:	f0 01 e0 5d 	5de001f0 addcc.l \$r0,\$r31
 318:	00 60 e0 5d 	5de06000 addcc.l \$r0,0 <pmlabel>
 31c:	00 c0 ef 5d 	5defc000 addcc.l \$r31,0 <pmlabel>
 320:	10 c0 ef 5d 	5defc010 addcc.l \$r31,1 <pmlabel\+0x1>
 324:	f0 df ef 5d 	5defdff0 addcc.l \$r31,1ff <pmlabel\+0x1ff>
 328:	10 80 ef 5b 	5bef8010 addcc.s \$r31,\$r1
 32c:	d0 c4 ef 5b 	5befc4d0 addcc.s \$r31,4d <pmlabel\+0x4d>
 330:	10 80 ef 59 	59ef8010 addcc.b \$r31,\$r1
 334:	d0 c4 ef 59 	59efc4d0 addcc.b \$r31,4d <pmlabel\+0x4d>
 338:	02 00 e0 5d 	5de00002 cmp.l \$r0,\$r0
 33c:	02 80 ef 5d 	5def8002 cmp.l \$r31,\$r0
 340:	f2 01 e0 5d 	5de001f2 cmp.l \$r0,\$r31
 344:	02 60 e0 5d 	5de06002 cmp.l \$r0,0 <pmlabel>
 348:	02 c0 ef 5d 	5defc002 cmp.l \$r31,0 <pmlabel>
 34c:	12 c0 ef 5d 	5defc012 cmp.l \$r31,1 <pmlabel\+0x1>
 350:	f2 df ef 5d 	5defdff2 cmp.l \$r31,1ff <pmlabel\+0x1ff>
 354:	12 80 ef 5b 	5bef8012 cmp.s \$r31,\$r1
 358:	d2 c4 ef 5b 	5befc4d2 cmp.s \$r31,4d <pmlabel\+0x4d>
 35c:	12 80 ef 59 	59ef8012 cmp.b \$r31,\$r1
 360:	d2 c4 ef 59 	59efc4d2 cmp.b \$r31,4d <pmlabel\+0x4d>
 364:	04 00 e0 5d 	5de00004 tst.l \$r0,\$r0
 368:	04 80 ef 5d 	5def8004 tst.l \$r31,\$r0
 36c:	f4 01 e0 5d 	5de001f4 tst.l \$r0,\$r31
 370:	04 60 e0 5d 	5de06004 tst.l \$r0,0 <pmlabel>
 374:	04 c0 ef 5d 	5defc004 tst.l \$r31,0 <pmlabel>
 378:	14 c0 ef 5d 	5defc014 tst.l \$r31,1 <pmlabel\+0x1>
 37c:	f4 df ef 5d 	5defdff4 tst.l \$r31,1ff <pmlabel\+0x1ff>
 380:	14 80 ef 5b 	5bef8014 tst.s \$r31,\$r1
 384:	d4 c4 ef 5b 	5befc4d4 tst.s \$r31,4d <pmlabel\+0x4d>
 388:	14 80 ef 59 	59ef8014 tst.b \$r31,\$r1
 38c:	d4 c4 ef 59 	59efc4d4 tst.b \$r31,4d <pmlabel\+0x4d>
 390:	0c 00 e0 5d 	5de0000c btst.l \$r0,\$r0
 394:	0c 80 ef 5d 	5def800c btst.l \$r31,\$r0
 398:	fc 01 e0 5d 	5de001fc btst.l \$r0,\$r31
 39c:	0c 60 e0 5d 	5de0600c btst.l \$r0,0 <pmlabel>
 3a0:	0c c0 ef 5d 	5defc00c btst.l \$r31,0 <pmlabel>
 3a4:	1c c0 ef 5d 	5defc01c btst.l \$r31,1 <pmlabel\+0x1>
 3a8:	fc df ef 5d 	5defdffc btst.l \$r31,1ff <pmlabel\+0x1ff>
 3ac:	1c 80 ef 5b 	5bef801c btst.s \$r31,\$r1
 3b0:	dc c4 ef 5b 	5befc4dc btst.s \$r31,4d <pmlabel\+0x4d>
 3b4:	1c 80 ef 59 	59ef801c btst.b \$r31,\$r1
 3b8:	dc c4 ef 59 	59efc4dc btst.b \$r31,4d <pmlabel\+0x4d>
 3bc:	80 80 0f ac 	ac0f8080 ldi.l \$r0,\$r31,80 <pmlabel\+0x80>
 3c0:	7f 00 f0 ad 	adf0007f ldi.l \$r31,\$r0,7f <pmlabel\+0x7f>
 3c4:	80 80 0f aa 	aa0f8080 ldi.s \$r0,\$r31,80 <pmlabel\+0x80>
 3c8:	7f 80 0f aa 	aa0f807f ldi.s \$r0,\$r31,7f <pmlabel\+0x7f>
 3cc:	80 00 f0 a9 	a9f00080 ldi.b \$r31,\$r0,80 <pmlabel\+0x80>
 3d0:	7f 00 f0 a9 	a9f0007f ldi.b \$r31,\$r0,7f <pmlabel\+0x7f>
 3d4:	80 00 f0 b5 	b5f00080 sti.l \$r31,80 <pmlabel\+0x80>,\$r0
 3d8:	7f 80 0f b4 	b40f807f sti.l \$r0,7f <pmlabel\+0x7f>,\$r31
 3dc:	80 00 f0 b3 	b3f00080 sti.s \$r31,80 <pmlabel\+0x80>,\$r0
 3e0:	7f 00 f0 b3 	b3f0007f sti.s \$r31,7f <pmlabel\+0x7f>,\$r0
 3e4:	80 80 0f b0 	b00f8080 sti.b \$r0,80 <pmlabel\+0x80>,\$r31
 3e8:	7f 80 0f b0 	b00f807f sti.b \$r0,7f <pmlabel\+0x7f>,\$r31
 3ec:	80 80 0f ec 	ec0f8080 exi.l \$r0,\$r31,80 <pmlabel\+0x80>
 3f0:	7f 00 f0 ed 	edf0007f exi.l \$r31,\$r0,7f <pmlabel\+0x7f>
 3f4:	80 80 0f ea 	ea0f8080 exi.s \$r0,\$r31,80 <pmlabel\+0x80>
 3f8:	7f 80 0f ea 	ea0f807f exi.s \$r0,\$r31,7f <pmlabel\+0x7f>
 3fc:	80 00 f0 e9 	e9f00080 exi.b \$r31,\$r0,80 <pmlabel\+0x80>
 400:	7f 00 f0 e9 	e9f0007f exi.b \$r31,\$r0,7f <pmlabel\+0x7f>
 404:	00 00 00 6c 	6c000000 lpm.l \$r0,0 <pmlabel>
 408:	00 00 00 6b 	6b000000 lpm.s \$r16,0 <pmlabel>
 40c:	00 00 f0 69 	69f00000 lpm.b \$r31,0 <pmlabel>
 410:	80 80 00 cc 	cc008080 lpmi.l \$r0,\$r1,80 <pmlabel\+0x80>
 414:	7f 80 00 cb 	cb00807f lpmi.s \$r16,\$r1,7f <pmlabel\+0x7f>
 418:	80 80 f0 c9 	c9f08080 lpmi.b \$r31,\$r1,80 <pmlabel\+0x80>
 41c:	00 00 30 00 	00300000 jmp 0 <pmlabel>
 420:	00 01 30 08 	08300100 jmpi \$r16
 424:	00 00 c8 07 	07c80000 jmpx 31,\$r28,1,0 <pmlabel>
 428:	00 00 20 00 	00200000 jmpc nz,0 <pmlabel>
 42c:	00 00 34 00 	00340000 call 0 <pmlabel>
 430:	00 01 34 08 	08340100 calli \$r16
 434:	00 00 cc 07 	07cc0000 callx 31,\$r28,1,0 <pmlabel>
 438:	00 00 24 00 	00240000 callc nz,0 <pmlabel>
 43c:	00 00 00 84 	84000000 push.l \$r0
 440:	00 00 08 84 	84080000 push.l \$r16
 444:	00 80 0f 84 	840f8000 push.l \$r31
 448:	00 00 00 8c 	8c000000 pop.l \$r0
 44c:	00 00 00 8d 	8d000000 pop.l \$r16
 450:	00 00 f0 8d 	8df00000 pop.l \$r31
 454:	00 00 00 94 	94000000 link \$r0,0 <pmlabel>
 458:	ff ff 00 95 	9500ffff link \$r16,ffff <pmlabel\+0xffff>
 45c:	f9 03 f0 95 	95f003f9 link \$r31,3f9 <pmlabel\+0x3f9>
 460:	00 00 00 98 	98000000 unlink \$r0
 464:	00 00 00 99 	99000000 unlink \$r16
 468:	00 00 f0 99 	99f00000 unlink \$r31
 46c:	00 00 00 a0 	a0000000 return 
 470:	00 00 00 a4 	a4000000 reti 
 474:	00 00 00 c4 	c4000000 lda.l \$r0,0 <pmlabel>
 478:	00 00 00 c3 	c3000000 lda.s \$r16,0 <pmlabel>
 47c:	00 00 f0 c1 	c1f00000 lda.b \$r31,0 <pmlabel>
 480:	00 00 00 bc 	bc000000 sta.l 0 <pmlabel>,\$r0
 484:	00 00 00 bb 	bb000000 sta.s 0 <pmlabel>,\$r16
 488:	00 00 f0 b9 	b9f00000 sta.b 0 <pmlabel>,\$r31
 48c:	00 00 00 3c 	3c000000 exa.l \$r0,0 <pmlabel>
 490:	00 00 00 3b 	3b000000 exa.s \$r16,0 <pmlabel>
 494:	00 00 f0 39 	39f00000 exa.b \$r31,0 <pmlabel>
 498:	00 00 08 64 	64080000 ldk.l \$r0,80000 <pmlabel\+0x80000>
 49c:	ff ff 07 64 	6407ffff ldk.l \$r0,7ffff <pmlabel\+0x7ffff>
 4a0:	00 00 00 64 	64000000 ldk.l \$r0,0 <pmlabel>
 4a4:	00 c0 0f 44 	440fc000 move.l \$r0,\$r31
 4a8:	00 40 f0 45 	45f04000 move.l \$r31,\$r0
 4ac:	00 00 f0 f5 	f5f00000 udiv.l \$r31,\$r0,\$r0
 4b0:	00 80 0f f4 	f40f8000 udiv.l \$r0,\$r31,\$r0
 4b4:	f0 01 00 f4 	f40001f0 udiv.l \$r0,\$r0,\$r31
 4b8:	40 00 11 f4 	f4110040 udiv.l \$r1,\$r2,\$r4
 4bc:	00 00 88 f4 	f4880000 udiv.l \$r8,\$r16,\$r0
 4c0:	00 60 f0 f5 	f5f06000 udiv.l \$r31,\$r0,0 <pmlabel>
 4c4:	00 c0 0f f4 	f40fc000 udiv.l \$r0,\$r31,0 <pmlabel>
 4c8:	10 c0 0f f4 	f40fc010 udiv.l \$r0,\$r31,1 <pmlabel\+0x1>
 4cc:	f0 df 0f f4 	f40fdff0 udiv.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 4d0:	10 80 0f f2 	f20f8010 udiv.s \$r0,\$r31,\$r1
 4d4:	d0 c4 0f f2 	f20fc4d0 udiv.s \$r0,\$r31,4d <pmlabel\+0x4d>
 4d8:	10 80 0f f0 	f00f8010 udiv.b \$r0,\$r31,\$r1
 4dc:	d0 c4 0f f0 	f00fc4d0 udiv.b \$r0,\$r31,4d <pmlabel\+0x4d>
 4e0:	01 00 f0 f5 	f5f00001 umod.l \$r31,\$r0,\$r0
 4e4:	01 80 0f f4 	f40f8001 umod.l \$r0,\$r31,\$r0
 4e8:	f1 01 00 f4 	f40001f1 umod.l \$r0,\$r0,\$r31
 4ec:	41 00 11 f4 	f4110041 umod.l \$r1,\$r2,\$r4
 4f0:	01 00 88 f4 	f4880001 umod.l \$r8,\$r16,\$r0
 4f4:	01 60 f0 f5 	f5f06001 umod.l \$r31,\$r0,0 <pmlabel>
 4f8:	01 c0 0f f4 	f40fc001 umod.l \$r0,\$r31,0 <pmlabel>
 4fc:	11 c0 0f f4 	f40fc011 umod.l \$r0,\$r31,1 <pmlabel\+0x1>
 500:	f1 df 0f f4 	f40fdff1 umod.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 504:	11 80 0f f2 	f20f8011 umod.s \$r0,\$r31,\$r1
 508:	d1 c4 0f f2 	f20fc4d1 umod.s \$r0,\$r31,4d <pmlabel\+0x4d>
 50c:	11 80 0f f0 	f00f8011 umod.b \$r0,\$r31,\$r1
 510:	d1 c4 0f f0 	f00fc4d1 umod.b \$r0,\$r31,4d <pmlabel\+0x4d>
 514:	02 00 f0 f5 	f5f00002 div.l \$r31,\$r0,\$r0
 518:	02 80 0f f4 	f40f8002 div.l \$r0,\$r31,\$r0
 51c:	f2 01 00 f4 	f40001f2 div.l \$r0,\$r0,\$r31
 520:	42 00 11 f4 	f4110042 div.l \$r1,\$r2,\$r4
 524:	02 00 88 f4 	f4880002 div.l \$r8,\$r16,\$r0
 528:	02 60 f0 f5 	f5f06002 div.l \$r31,\$r0,0 <pmlabel>
 52c:	02 c0 0f f4 	f40fc002 div.l \$r0,\$r31,0 <pmlabel>
 530:	12 c0 0f f4 	f40fc012 div.l \$r0,\$r31,1 <pmlabel\+0x1>
 534:	f2 df 0f f4 	f40fdff2 div.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 538:	12 80 0f f2 	f20f8012 div.s \$r0,\$r31,\$r1
 53c:	d2 c4 0f f2 	f20fc4d2 div.s \$r0,\$r31,4d <pmlabel\+0x4d>
 540:	12 80 0f f0 	f00f8012 div.b \$r0,\$r31,\$r1
 544:	d2 c4 0f f0 	f00fc4d2 div.b \$r0,\$r31,4d <pmlabel\+0x4d>
 548:	03 00 f0 f5 	f5f00003 mod.l \$r31,\$r0,\$r0
 54c:	03 80 0f f4 	f40f8003 mod.l \$r0,\$r31,\$r0
 550:	f3 01 00 f4 	f40001f3 mod.l \$r0,\$r0,\$r31
 554:	43 00 11 f4 	f4110043 mod.l \$r1,\$r2,\$r4
 558:	03 00 88 f4 	f4880003 mod.l \$r8,\$r16,\$r0
 55c:	03 60 f0 f5 	f5f06003 mod.l \$r31,\$r0,0 <pmlabel>
 560:	03 c0 0f f4 	f40fc003 mod.l \$r0,\$r31,0 <pmlabel>
 564:	13 c0 0f f4 	f40fc013 mod.l \$r0,\$r31,1 <pmlabel\+0x1>
 568:	f3 df 0f f4 	f40fdff3 mod.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 56c:	13 80 0f f2 	f20f8013 mod.s \$r0,\$r31,\$r1
 570:	d3 c4 0f f2 	f20fc4d3 mod.s \$r0,\$r31,4d <pmlabel\+0x4d>
 574:	13 80 0f f0 	f00f8013 mod.b \$r0,\$r31,\$r1
 578:	d3 c4 0f f0 	f00fc4d3 mod.b \$r0,\$r31,4d <pmlabel\+0x4d>
 57c:	04 00 f0 f5 	f5f00004 strcmp.l \$r31,\$r0,\$r0
 580:	04 80 0f f4 	f40f8004 strcmp.l \$r0,\$r31,\$r0
 584:	f4 01 00 f4 	f40001f4 strcmp.l \$r0,\$r0,\$r31
 588:	44 00 11 f4 	f4110044 strcmp.l \$r1,\$r2,\$r4
 58c:	04 00 88 f4 	f4880004 strcmp.l \$r8,\$r16,\$r0
 590:	04 60 f0 f5 	f5f06004 strcmp.l \$r31,\$r0,0 <pmlabel>
 594:	04 c0 0f f4 	f40fc004 strcmp.l \$r0,\$r31,0 <pmlabel>
 598:	14 c0 0f f4 	f40fc014 strcmp.l \$r0,\$r31,1 <pmlabel\+0x1>
 59c:	f4 df 0f f4 	f40fdff4 strcmp.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 5a0:	14 80 0f f2 	f20f8014 strcmp.s \$r0,\$r31,\$r1
 5a4:	d4 c4 0f f2 	f20fc4d4 strcmp.s \$r0,\$r31,4d <pmlabel\+0x4d>
 5a8:	14 80 0f f0 	f00f8014 strcmp.b \$r0,\$r31,\$r1
 5ac:	d4 c4 0f f0 	f00fc4d4 strcmp.b \$r0,\$r31,4d <pmlabel\+0x4d>
 5b0:	05 00 f0 f5 	f5f00005 memcpy.l \$r31,\$r0,\$r0
 5b4:	05 80 0f f4 	f40f8005 memcpy.l \$r0,\$r31,\$r0
 5b8:	f5 01 00 f4 	f40001f5 memcpy.l \$r0,\$r0,\$r31
 5bc:	45 00 11 f4 	f4110045 memcpy.l \$r1,\$r2,\$r4
 5c0:	05 00 88 f4 	f4880005 memcpy.l \$r8,\$r16,\$r0
 5c4:	05 60 f0 f5 	f5f06005 memcpy.l \$r31,\$r0,0 <pmlabel>
 5c8:	05 c0 0f f4 	f40fc005 memcpy.l \$r0,\$r31,0 <pmlabel>
 5cc:	15 c0 0f f4 	f40fc015 memcpy.l \$r0,\$r31,1 <pmlabel\+0x1>
 5d0:	f5 df 0f f4 	f40fdff5 memcpy.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 5d4:	15 80 0f f2 	f20f8015 memcpy.s \$r0,\$r31,\$r1
 5d8:	d5 c4 0f f2 	f20fc4d5 memcpy.s \$r0,\$r31,4d <pmlabel\+0x4d>
 5dc:	15 80 0f f0 	f00f8015 memcpy.b \$r0,\$r31,\$r1
 5e0:	d5 c4 0f f0 	f00fc4d5 memcpy.b \$r0,\$r31,4d <pmlabel\+0x4d>
 5e4:	07 00 f0 f5 	f5f00007 memset.l \$r31,\$r0,\$r0
 5e8:	07 80 0f f4 	f40f8007 memset.l \$r0,\$r31,\$r0
 5ec:	f7 01 00 f4 	f40001f7 memset.l \$r0,\$r0,\$r31
 5f0:	47 00 11 f4 	f4110047 memset.l \$r1,\$r2,\$r4
 5f4:	07 00 88 f4 	f4880007 memset.l \$r8,\$r16,\$r0
 5f8:	07 60 f0 f5 	f5f06007 memset.l \$r31,\$r0,0 <pmlabel>
 5fc:	07 c0 0f f4 	f40fc007 memset.l \$r0,\$r31,0 <pmlabel>
 600:	17 c0 0f f4 	f40fc017 memset.l \$r0,\$r31,1 <pmlabel\+0x1>
 604:	f7 df 0f f4 	f40fdff7 memset.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 608:	17 80 0f f2 	f20f8017 memset.s \$r0,\$r31,\$r1
 60c:	d7 c4 0f f2 	f20fc4d7 memset.s \$r0,\$r31,4d <pmlabel\+0x4d>
 610:	17 80 0f f0 	f00f8017 memset.b \$r0,\$r31,\$r1
 614:	d7 c4 0f f0 	f00fc4d7 memset.b \$r0,\$r31,4d <pmlabel\+0x4d>
 618:	08 00 f0 f5 	f5f00008 mul.l \$r31,\$r0,\$r0
 61c:	08 80 0f f4 	f40f8008 mul.l \$r0,\$r31,\$r0
 620:	f8 01 00 f4 	f40001f8 mul.l \$r0,\$r0,\$r31
 624:	48 00 11 f4 	f4110048 mul.l \$r1,\$r2,\$r4
 628:	08 00 88 f4 	f4880008 mul.l \$r8,\$r16,\$r0
 62c:	08 60 f0 f5 	f5f06008 mul.l \$r31,\$r0,0 <pmlabel>
 630:	08 c0 0f f4 	f40fc008 mul.l \$r0,\$r31,0 <pmlabel>
 634:	18 c0 0f f4 	f40fc018 mul.l \$r0,\$r31,1 <pmlabel\+0x1>
 638:	f8 df 0f f4 	f40fdff8 mul.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 63c:	18 80 0f f2 	f20f8018 mul.s \$r0,\$r31,\$r1
 640:	d8 c4 0f f2 	f20fc4d8 mul.s \$r0,\$r31,4d <pmlabel\+0x4d>
 644:	18 80 0f f0 	f00f8018 mul.b \$r0,\$r31,\$r1
 648:	d8 c4 0f f0 	f00fc4d8 mul.b \$r0,\$r31,4d <pmlabel\+0x4d>
 64c:	09 00 f0 f5 	f5f00009 muluh.l \$r31,\$r0,\$r0
 650:	09 80 0f f4 	f40f8009 muluh.l \$r0,\$r31,\$r0
 654:	f9 01 00 f4 	f40001f9 muluh.l \$r0,\$r0,\$r31
 658:	49 00 11 f4 	f4110049 muluh.l \$r1,\$r2,\$r4
 65c:	09 00 88 f4 	f4880009 muluh.l \$r8,\$r16,\$r0
 660:	09 60 f0 f5 	f5f06009 muluh.l \$r31,\$r0,0 <pmlabel>
 664:	09 c0 0f f4 	f40fc009 muluh.l \$r0,\$r31,0 <pmlabel>
 668:	19 c0 0f f4 	f40fc019 muluh.l \$r0,\$r31,1 <pmlabel\+0x1>
 66c:	f9 df 0f f4 	f40fdff9 muluh.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 670:	19 80 0f f2 	f20f8019 muluh.s \$r0,\$r31,\$r1
 674:	d9 c4 0f f2 	f20fc4d9 muluh.s \$r0,\$r31,4d <pmlabel\+0x4d>
 678:	19 80 0f f0 	f00f8019 muluh.b \$r0,\$r31,\$r1
 67c:	d9 c4 0f f0 	f00fc4d9 muluh.b \$r0,\$r31,4d <pmlabel\+0x4d>
 680:	0c 00 f0 f5 	f5f0000c streamin.l \$r31,\$r0,\$r0
 684:	0c 80 0f f4 	f40f800c streamin.l \$r0,\$r31,\$r0
 688:	fc 01 00 f4 	f40001fc streamin.l \$r0,\$r0,\$r31
 68c:	4c 00 11 f4 	f411004c streamin.l \$r1,\$r2,\$r4
 690:	0c 00 88 f4 	f488000c streamin.l \$r8,\$r16,\$r0
 694:	0c 60 f0 f5 	f5f0600c streamin.l \$r31,\$r0,0 <pmlabel>
 698:	0c c0 0f f4 	f40fc00c streamin.l \$r0,\$r31,0 <pmlabel>
 69c:	1c c0 0f f4 	f40fc01c streamin.l \$r0,\$r31,1 <pmlabel\+0x1>
 6a0:	fc df 0f f4 	f40fdffc streamin.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 6a4:	1c 80 0f f2 	f20f801c streamin.s \$r0,\$r31,\$r1
 6a8:	dc c4 0f f2 	f20fc4dc streamin.s \$r0,\$r31,4d <pmlabel\+0x4d>
 6ac:	1c 80 0f f0 	f00f801c streamin.b \$r0,\$r31,\$r1
 6b0:	dc c4 0f f0 	f00fc4dc streamin.b \$r0,\$r31,4d <pmlabel\+0x4d>
 6b4:	0d 00 f0 f5 	f5f0000d streamini.l \$r31,\$r0,\$r0
 6b8:	0d 80 0f f4 	f40f800d streamini.l \$r0,\$r31,\$r0
 6bc:	fd 01 00 f4 	f40001fd streamini.l \$r0,\$r0,\$r31
 6c0:	4d 00 11 f4 	f411004d streamini.l \$r1,\$r2,\$r4
 6c4:	0d 00 88 f4 	f488000d streamini.l \$r8,\$r16,\$r0
 6c8:	0d 60 f0 f5 	f5f0600d streamini.l \$r31,\$r0,0 <pmlabel>
 6cc:	0d c0 0f f4 	f40fc00d streamini.l \$r0,\$r31,0 <pmlabel>
 6d0:	1d c0 0f f4 	f40fc01d streamini.l \$r0,\$r31,1 <pmlabel\+0x1>
 6d4:	fd df 0f f4 	f40fdffd streamini.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 6d8:	1d 80 0f f2 	f20f801d streamini.s \$r0,\$r31,\$r1
 6dc:	dd c4 0f f2 	f20fc4dd streamini.s \$r0,\$r31,4d <pmlabel\+0x4d>
 6e0:	1d 80 0f f0 	f00f801d streamini.b \$r0,\$r31,\$r1
 6e4:	dd c4 0f f0 	f00fc4dd streamini.b \$r0,\$r31,4d <pmlabel\+0x4d>
 6e8:	0e 00 f0 f5 	f5f0000e streamout.l \$r31,\$r0,\$r0
 6ec:	0e 80 0f f4 	f40f800e streamout.l \$r0,\$r31,\$r0
 6f0:	fe 01 00 f4 	f40001fe streamout.l \$r0,\$r0,\$r31
 6f4:	4e 00 11 f4 	f411004e streamout.l \$r1,\$r2,\$r4
 6f8:	0e 00 88 f4 	f488000e streamout.l \$r8,\$r16,\$r0
 6fc:	0e 60 f0 f5 	f5f0600e streamout.l \$r31,\$r0,0 <pmlabel>
 700:	0e c0 0f f4 	f40fc00e streamout.l \$r0,\$r31,0 <pmlabel>
 704:	1e c0 0f f4 	f40fc01e streamout.l \$r0,\$r31,1 <pmlabel\+0x1>
 708:	fe df 0f f4 	f40fdffe streamout.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 70c:	1e 80 0f f2 	f20f801e streamout.s \$r0,\$r31,\$r1
 710:	de c4 0f f2 	f20fc4de streamout.s \$r0,\$r31,4d <pmlabel\+0x4d>
 714:	1e 80 0f f0 	f00f801e streamout.b \$r0,\$r31,\$r1
 718:	de c4 0f f0 	f00fc4de streamout.b \$r0,\$r31,4d <pmlabel\+0x4d>
 71c:	0f 00 f0 f5 	f5f0000f streamouti.l \$r31,\$r0,\$r0
 720:	0f 80 0f f4 	f40f800f streamouti.l \$r0,\$r31,\$r0
 724:	ff 01 00 f4 	f40001ff streamouti.l \$r0,\$r0,\$r31
 728:	4f 00 11 f4 	f411004f streamouti.l \$r1,\$r2,\$r4
 72c:	0f 00 88 f4 	f488000f streamouti.l \$r8,\$r16,\$r0
 730:	0f 60 f0 f5 	f5f0600f streamouti.l \$r31,\$r0,0 <pmlabel>
 734:	0f c0 0f f4 	f40fc00f streamouti.l \$r0,\$r31,0 <pmlabel>
 738:	1f c0 0f f4 	f40fc01f streamouti.l \$r0,\$r31,1 <pmlabel\+0x1>
 73c:	ff df 0f f4 	f40fdfff streamouti.l \$r0,\$r31,1ff <pmlabel\+0x1ff>
 740:	1f 80 0f f2 	f20f801f streamouti.s \$r0,\$r31,\$r1
 744:	df c4 0f f2 	f20fc4df streamouti.s \$r0,\$r31,4d <pmlabel\+0x4d>
 748:	1f 80 0f f0 	f00f801f streamouti.b \$r0,\$r31,\$r1
 74c:	df c4 0f f0 	f00fc4df streamouti.b \$r0,\$r31,4d <pmlabel\+0x4d>
 750:	06 c0 0f f4 	f40fc006 strlen.l \$r0,\$r31
 754:	06 40 f0 f5 	f5f04006 strlen.l \$r31,\$r0
 758:	06 c0 0f f2 	f20fc006 strlen.s \$r0,\$r31
 75c:	06 40 f0 f3 	f3f04006 strlen.s \$r31,\$r0
 760:	06 c0 0f f0 	f00fc006 strlen.b \$r0,\$r31
 764:	06 40 f0 f1 	f1f04006 strlen.b \$r31,\$r0
 768:	0a c0 0f f4 	f40fc00a stpcpy.l \$r0,\$r31
 76c:	0a 40 f0 f5 	f5f0400a stpcpy.l \$r31,\$r0
 770:	0a c0 0f f2 	f20fc00a stpcpy.s \$r0,\$r31
 774:	0a 40 f0 f3 	f3f0400a stpcpy.s \$r31,\$r0
 778:	0a c0 0f f0 	f00fc00a stpcpy.b \$r0,\$r31
 77c:	0a 40 f0 f1 	f1f0400a stpcpy.b \$r31,\$r0
