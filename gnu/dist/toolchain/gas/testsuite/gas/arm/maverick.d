#objdump: -dr --prefix-address --show-raw-insn
#name: Maverick
#as: -mcpu=arm9+maverick

# Test the instructions of Maverick

.*: +file format.*arm.*

Disassembly of section .text:
# load_store:
00000000 <load_store> 0d ?9d ?54 ?ff ? *	cfldrseq	mvf5, ?\[sp, ?#255\]
00000004 <load_store\+0x4> 4d ?9b ?e4 ?49 ? *	cfldrsmi	mvf14, ?\[r11, ?#73\]
00000008 <load_store\+0x8> 7d ?1c ?24 ?ef ? *	cfldrsvc	mvf2, ?\[r12, ?#-239\]
0000000c <load_store\+0xc> bd ?1a ?04 ?ff ? *	cfldrslt	mvf0, ?\[r10, ?#-255\]
00000010 <load_store\+0x10> 3d ?11 ?c4 ?27 ? *	cfldrscc	mvf12, ?\[r1, ?#-39\]
00000014 <load_store\+0x14> ed ?bf ?d4 ?68 ? *	cfldrs	mvf13, ?\[pc, ?#104\]!
00000018 <load_store\+0x18> 2d ?30 ?94 ?00 ? *	cfldrscs	mvf9, ?\[r0, ?#-0\]!
0000001c <load_store\+0x1c> ad ?be ?94 ?48 ? *	cfldrsge	mvf9, ?\[lr, ?#72\]!
00000020 <load_store\+0x20> 8d ?b5 ?d4 ?25 ? *	cfldrshi	mvf13, ?\[r5, ?#37\]!
00000024 <load_store\+0x24> cd ?b3 ?64 ?00 ? *	cfldrsgt	mvf6, ?\[r3, ?#0\]!
00000028 <load_store\+0x28> 5c ?94 ?e4 ?40 ? *	cfldrspl	mvf14, ?\[r4\], ?#64
0000002c <load_store\+0x2c> 1c ?12 ?84 ?9d ? *	cfldrsne	mvf8, ?\[r2\], ?#-157
00000030 <load_store\+0x30> bc ?99 ?44 ?01 ? *	cfldrslt	mvf4, ?\[r9\], ?#1
00000034 <load_store\+0x34> 5c ?17 ?f4 ?3f ? *	cfldrspl	mvf15, ?\[r7\], ?#-63
00000038 <load_store\+0x38> ec ?18 ?34 ?88 ? *	cfldrs	mvf3, ?\[r8\], ?#-136
0000003c <load_store\+0x3c> 2d ?56 ?14 ?44 ? *	cfldrdcs	mvd1, ?\[r6, ?#-68\]
00000040 <load_store\+0x40> 0d ?dd ?74 ?ff ? *	cfldrdeq	mvd7, ?\[sp, ?#255\]
00000044 <load_store\+0x44> cd ?db ?a4 ?49 ? *	cfldrdgt	mvd10, ?\[r11, ?#73\]
00000048 <load_store\+0x48> dd ?5c ?64 ?ef ? *	cfldrdle	mvd6, ?\[r12, ?#-239\]
0000004c <load_store\+0x4c> 9d ?5a ?04 ?ff ? *	cfldrdls	mvd0, ?\[r10, ?#-255\]
00000050 <load_store\+0x50> 9d ?71 ?44 ?27 ? *	cfldrdls	mvd4, ?\[r1, ?#-39\]!
00000054 <load_store\+0x54> dd ?ff ?74 ?68 ? *	cfldrdle	mvd7, ?\[pc, ?#104\]!
00000058 <load_store\+0x58> 6d ?70 ?b4 ?00 ? *	cfldrdvs	mvd11, ?\[r0, ?#-0\]!
0000005c <load_store\+0x5c> ed ?fe ?34 ?48 ? *	cfldrd	mvd3, ?\[lr, ?#72\]!
00000060 <load_store\+0x60> 8d ?f5 ?f4 ?25 ? *	cfldrdhi	mvd15, ?\[r5, ?#37\]!
00000064 <load_store\+0x64> 4c ?d3 ?24 ?00 ? *	cfldrdmi	mvd2, ?\[r3\], ?#0
00000068 <load_store\+0x68> ec ?d4 ?a4 ?40 ? *	cfldrd	mvd10, ?\[r4\], ?#64
0000006c <load_store\+0x6c> 3c ?52 ?84 ?9d ? *	cfldrdcc	mvd8, ?\[r2\], ?#-157
00000070 <load_store\+0x70> 1c ?d9 ?c4 ?01 ? *	cfldrdne	mvd12, ?\[r9\], ?#1
00000074 <load_store\+0x74> 7c ?57 ?54 ?3f ? *	cfldrdvc	mvd5, ?\[r7\], ?#-63
00000078 <load_store\+0x78> ad ?18 ?15 ?88 ? *	cfldr32ge	mvfx1, ?\[r8, ?#-136\]
0000007c <load_store\+0x7c> 6d ?16 ?b5 ?44 ? *	cfldr32vs	mvfx11, ?\[r6, ?#-68\]
00000080 <load_store\+0x80> 0d ?9d ?55 ?ff ? *	cfldr32eq	mvfx5, ?\[sp, ?#255\]
00000084 <load_store\+0x84> 4d ?9b ?e5 ?49 ? *	cfldr32mi	mvfx14, ?\[r11, ?#73\]
00000088 <load_store\+0x88> 7d ?1c ?25 ?ef ? *	cfldr32vc	mvfx2, ?\[r12, ?#-239\]
0000008c <load_store\+0x8c> bd ?3a ?05 ?ff ? *	cfldr32lt	mvfx0, ?\[r10, ?#-255\]!
00000090 <load_store\+0x90> 3d ?31 ?c5 ?27 ? *	cfldr32cc	mvfx12, ?\[r1, ?#-39\]!
00000094 <load_store\+0x94> ed ?bf ?d5 ?68 ? *	cfldr32	mvfx13, ?\[pc, ?#104\]!
00000098 <load_store\+0x98> 2d ?30 ?95 ?00 ? *	cfldr32cs	mvfx9, ?\[r0, ?#-0\]!
0000009c <load_store\+0x9c> ad ?be ?95 ?48 ? *	cfldr32ge	mvfx9, ?\[lr, ?#72\]!
000000a0 <load_store\+0xa0> 8c ?95 ?d5 ?25 ? *	cfldr32hi	mvfx13, ?\[r5\], ?#37
000000a4 <load_store\+0xa4> cc ?93 ?65 ?00 ? *	cfldr32gt	mvfx6, ?\[r3\], ?#0
000000a8 <load_store\+0xa8> 5c ?94 ?e5 ?40 ? *	cfldr32pl	mvfx14, ?\[r4\], ?#64
000000ac <load_store\+0xac> 1c ?12 ?85 ?9d ? *	cfldr32ne	mvfx8, ?\[r2\], ?#-157
000000b0 <load_store\+0xb0> bc ?99 ?45 ?01 ? *	cfldr32lt	mvfx4, ?\[r9\], ?#1
000000b4 <load_store\+0xb4> 5d ?57 ?f5 ?3f ? *	cfldr64pl	mvdx15, ?\[r7, ?#-63\]
000000b8 <load_store\+0xb8> ed ?58 ?35 ?88 ? *	cfldr64	mvdx3, ?\[r8, ?#-136\]
000000bc <load_store\+0xbc> 2d ?56 ?15 ?44 ? *	cfldr64cs	mvdx1, ?\[r6, ?#-68\]
000000c0 <load_store\+0xc0> 0d ?dd ?75 ?ff ? *	cfldr64eq	mvdx7, ?\[sp, ?#255\]
000000c4 <load_store\+0xc4> cd ?db ?a5 ?49 ? *	cfldr64gt	mvdx10, ?\[r11, ?#73\]
000000c8 <load_store\+0xc8> dd ?7c ?65 ?ef ? *	cfldr64le	mvdx6, ?\[r12, ?#-239\]!
000000cc <load_store\+0xcc> 9d ?7a ?05 ?ff ? *	cfldr64ls	mvdx0, ?\[r10, ?#-255\]!
000000d0 <load_store\+0xd0> 9d ?71 ?45 ?27 ? *	cfldr64ls	mvdx4, ?\[r1, ?#-39\]!
000000d4 <load_store\+0xd4> dd ?ff ?75 ?68 ? *	cfldr64le	mvdx7, ?\[pc, ?#104\]!
000000d8 <load_store\+0xd8> 6d ?70 ?b5 ?00 ? *	cfldr64vs	mvdx11, ?\[r0, ?#-0\]!
000000dc <load_store\+0xdc> ec ?de ?35 ?48 ? *	cfldr64	mvdx3, ?\[lr\], ?#72
000000e0 <load_store\+0xe0> 8c ?d5 ?f5 ?25 ? *	cfldr64hi	mvdx15, ?\[r5\], ?#37
000000e4 <load_store\+0xe4> 4c ?d3 ?25 ?00 ? *	cfldr64mi	mvdx2, ?\[r3\], ?#0
000000e8 <load_store\+0xe8> ec ?d4 ?a5 ?40 ? *	cfldr64	mvdx10, ?\[r4\], ?#64
000000ec <load_store\+0xec> 3c ?52 ?85 ?9d ? *	cfldr64cc	mvdx8, ?\[r2\], ?#-157
000000f0 <load_store\+0xf0> 1d ?89 ?c4 ?01 ? *	cfstrsne	mvf12, ?\[r9, ?#1\]
000000f4 <load_store\+0xf4> 7d ?07 ?54 ?3f ? *	cfstrsvc	mvf5, ?\[r7, ?#-63\]
000000f8 <load_store\+0xf8> ad ?08 ?14 ?88 ? *	cfstrsge	mvf1, ?\[r8, ?#-136\]
000000fc <load_store\+0xfc> 6d ?06 ?b4 ?44 ? *	cfstrsvs	mvf11, ?\[r6, ?#-68\]
00000100 <load_store\+0x100> 0d ?8d ?54 ?ff ? *	cfstrseq	mvf5, ?\[sp, ?#255\]
00000104 <load_store\+0x104> 4d ?ab ?e4 ?49 ? *	cfstrsmi	mvf14, ?\[r11, ?#73\]!
00000108 <load_store\+0x108> 7d ?2c ?24 ?ef ? *	cfstrsvc	mvf2, ?\[r12, ?#-239\]!
0000010c <load_store\+0x10c> bd ?2a ?04 ?ff ? *	cfstrslt	mvf0, ?\[r10, ?#-255\]!
00000110 <load_store\+0x110> 3d ?21 ?c4 ?27 ? *	cfstrscc	mvf12, ?\[r1, ?#-39\]!
00000114 <load_store\+0x114> ed ?af ?d4 ?68 ? *	cfstrs	mvf13, ?\[pc, ?#104\]!
00000118 <load_store\+0x118> 2c ?00 ?94 ?00 ? *	cfstrscs	mvf9, ?\[r0\], ?#-0
0000011c <load_store\+0x11c> ac ?8e ?94 ?48 ? *	cfstrsge	mvf9, ?\[lr\], ?#72
00000120 <load_store\+0x120> 8c ?85 ?d4 ?25 ? *	cfstrshi	mvf13, ?\[r5\], ?#37
00000124 <load_store\+0x124> cc ?83 ?64 ?00 ? *	cfstrsgt	mvf6, ?\[r3\], ?#0
00000128 <load_store\+0x128> 5c ?84 ?e4 ?40 ? *	cfstrspl	mvf14, ?\[r4\], ?#64
0000012c <load_store\+0x12c> 1d ?42 ?84 ?9d ? *	cfstrdne	mvd8, ?\[r2, ?#-157\]
00000130 <load_store\+0x130> bd ?c9 ?44 ?01 ? *	cfstrdlt	mvd4, ?\[r9, ?#1\]
00000134 <load_store\+0x134> 5d ?47 ?f4 ?3f ? *	cfstrdpl	mvd15, ?\[r7, ?#-63\]
00000138 <load_store\+0x138> ed ?48 ?34 ?88 ? *	cfstrd	mvd3, ?\[r8, ?#-136\]
0000013c <load_store\+0x13c> 2d ?46 ?14 ?44 ? *	cfstrdcs	mvd1, ?\[r6, ?#-68\]
00000140 <load_store\+0x140> 0d ?ed ?74 ?ff ? *	cfstrdeq	mvd7, ?\[sp, ?#255\]!
00000144 <load_store\+0x144> cd ?eb ?a4 ?49 ? *	cfstrdgt	mvd10, ?\[r11, ?#73\]!
00000148 <load_store\+0x148> dd ?6c ?64 ?ef ? *	cfstrdle	mvd6, ?\[r12, ?#-239\]!
0000014c <load_store\+0x14c> 9d ?6a ?04 ?ff ? *	cfstrdls	mvd0, ?\[r10, ?#-255\]!
00000150 <load_store\+0x150> 9d ?61 ?44 ?27 ? *	cfstrdls	mvd4, ?\[r1, ?#-39\]!
00000154 <load_store\+0x154> dc ?cf ?74 ?68 ? *	cfstrdle	mvd7, ?\[pc\], ?#104
00000158 <load_store\+0x158> 6c ?40 ?b4 ?00 ? *	cfstrdvs	mvd11, ?\[r0\], ?#-0
0000015c <load_store\+0x15c> ec ?ce ?34 ?48 ? *	cfstrd	mvd3, ?\[lr\], ?#72
00000160 <load_store\+0x160> 8c ?c5 ?f4 ?25 ? *	cfstrdhi	mvd15, ?\[r5\], ?#37
00000164 <load_store\+0x164> 4c ?c3 ?24 ?00 ? *	cfstrdmi	mvd2, ?\[r3\], ?#0
00000168 <load_store\+0x168> ed ?84 ?a5 ?40 ? *	cfstr32	mvfx10, ?\[r4, ?#64\]
0000016c <load_store\+0x16c> 3d ?02 ?85 ?9d ? *	cfstr32cc	mvfx8, ?\[r2, ?#-157\]
00000170 <load_store\+0x170> 1d ?89 ?c5 ?01 ? *	cfstr32ne	mvfx12, ?\[r9, ?#1\]
00000174 <load_store\+0x174> 7d ?07 ?55 ?3f ? *	cfstr32vc	mvfx5, ?\[r7, ?#-63\]
00000178 <load_store\+0x178> ad ?08 ?15 ?88 ? *	cfstr32ge	mvfx1, ?\[r8, ?#-136\]
0000017c <load_store\+0x17c> 6d ?26 ?b5 ?44 ? *	cfstr32vs	mvfx11, ?\[r6, ?#-68\]!
00000180 <load_store\+0x180> 0d ?ad ?55 ?ff ? *	cfstr32eq	mvfx5, ?\[sp, ?#255\]!
00000184 <load_store\+0x184> 4d ?ab ?e5 ?49 ? *	cfstr32mi	mvfx14, ?\[r11, ?#73\]!
00000188 <load_store\+0x188> 7d ?2c ?25 ?ef ? *	cfstr32vc	mvfx2, ?\[r12, ?#-239\]!
0000018c <load_store\+0x18c> bd ?2a ?05 ?ff ? *	cfstr32lt	mvfx0, ?\[r10, ?#-255\]!
00000190 <load_store\+0x190> 3c ?01 ?c5 ?27 ? *	cfstr32cc	mvfx12, ?\[r1\], ?#-39
00000194 <load_store\+0x194> ec ?8f ?d5 ?68 ? *	cfstr32	mvfx13, ?\[pc\], ?#104
00000198 <load_store\+0x198> 2c ?00 ?95 ?00 ? *	cfstr32cs	mvfx9, ?\[r0\], ?#-0
0000019c <load_store\+0x19c> ac ?8e ?95 ?48 ? *	cfstr32ge	mvfx9, ?\[lr\], ?#72
000001a0 <load_store\+0x1a0> 8c ?85 ?d5 ?25 ? *	cfstr32hi	mvfx13, ?\[r5\], ?#37
000001a4 <load_store\+0x1a4> cd ?c3 ?65 ?00 ? *	cfstr64gt	mvdx6, ?\[r3, ?#0\]
000001a8 <load_store\+0x1a8> 5d ?c4 ?e5 ?40 ? *	cfstr64pl	mvdx14, ?\[r4, ?#64\]
000001ac <load_store\+0x1ac> 1d ?42 ?85 ?9d ? *	cfstr64ne	mvdx8, ?\[r2, ?#-157\]
000001b0 <load_store\+0x1b0> bd ?c9 ?45 ?01 ? *	cfstr64lt	mvdx4, ?\[r9, ?#1\]
000001b4 <load_store\+0x1b4> 5d ?47 ?f5 ?3f ? *	cfstr64pl	mvdx15, ?\[r7, ?#-63\]
000001b8 <load_store\+0x1b8> ed ?68 ?35 ?88 ? *	cfstr64	mvdx3, ?\[r8, ?#-136\]!
000001bc <load_store\+0x1bc> 2d ?66 ?15 ?44 ? *	cfstr64cs	mvdx1, ?\[r6, ?#-68\]!
000001c0 <load_store\+0x1c0> 0d ?ed ?75 ?ff ? *	cfstr64eq	mvdx7, ?\[sp, ?#255\]!
000001c4 <load_store\+0x1c4> cd ?eb ?a5 ?49 ? *	cfstr64gt	mvdx10, ?\[r11, ?#73\]!
000001c8 <load_store\+0x1c8> dd ?6c ?65 ?ef ? *	cfstr64le	mvdx6, ?\[r12, ?#-239\]!
000001cc <load_store\+0x1cc> 9c ?4a ?05 ?ff ? *	cfstr64ls	mvdx0, ?\[r10\], ?#-255
000001d0 <load_store\+0x1d0> 9c ?41 ?45 ?27 ? *	cfstr64ls	mvdx4, ?\[r1\], ?#-39
000001d4 <load_store\+0x1d4> dc ?cf ?75 ?68 ? *	cfstr64le	mvdx7, ?\[pc\], ?#104
000001d8 <load_store\+0x1d8> 6c ?40 ?b5 ?00 ? *	cfstr64vs	mvdx11, ?\[r0\], ?#-0
000001dc <load_store\+0x1dc> ec ?ce ?35 ?48 ? *	cfstr64	mvdx3, ?\[lr\], ?#72
# move:
000001e0 <move> 8e ?0f ?54 ?50 ? *	cfmvsrhi	mvf15, ?r5
000001e4 <move\+0x4> 6e ?0b ?64 ?50 ? *	cfmvsrvs	mvf11, ?r6
000001e8 <move\+0x8> 2e ?09 ?04 ?50 ? *	cfmvsrcs	mvf9, ?r0
000001ec <move\+0xc> 5e ?0f ?74 ?50 ? *	cfmvsrpl	mvf15, ?r7
000001f0 <move\+0x10> 9e ?04 ?14 ?50 ? *	cfmvsrls	mvf4, ?r1
000001f4 <move\+0x14> 3e ?1d ?84 ?50 ? *	cfmvrscc	r8, ?mvf13
000001f8 <move\+0x18> 7e ?11 ?f4 ?50 ? *	cfmvrsvc	pc, ?mvf1
000001fc <move\+0x1c> ce ?1b ?94 ?50 ? *	cfmvrsgt	r9, ?mvf11
00000200 <move\+0x20> 0e ?15 ?a4 ?50 ? *	cfmvrseq	r10, ?mvf5
00000204 <move\+0x24> ee ?1c ?44 ?50 ? *	cfmvrs	r4, ?mvf12
00000208 <move\+0x28> ae ?01 ?84 ?10 ? *	cfmvdlrge	mvd1, ?r8
0000020c <move\+0x2c> ee ?0d ?f4 ?10 ? *	cfmvdlr	mvd13, ?pc
00000210 <move\+0x30> be ?04 ?94 ?10 ? *	cfmvdlrlt	mvd4, ?r9
00000214 <move\+0x34> 9e ?00 ?a4 ?10 ? *	cfmvdlrls	mvd0, ?r10
00000218 <move\+0x38> ee ?0a ?44 ?10 ? *	cfmvdlr	mvd10, ?r4
0000021c <move\+0x3c> 4e ?13 ?14 ?10 ? *	cfmvrdlmi	r1, ?mvd3
00000220 <move\+0x40> 8e ?17 ?24 ?10 ? *	cfmvrdlhi	r2, ?mvd7
00000224 <move\+0x44> 2e ?1c ?c4 ?10 ? *	cfmvrdlcs	r12, ?mvd12
00000228 <move\+0x48> 6e ?10 ?34 ?10 ? *	cfmvrdlvs	r3, ?mvd0
0000022c <move\+0x4c> 7e ?1e ?d4 ?10 ? *	cfmvrdlvc	sp, ?mvd14
00000230 <move\+0x50> 3e ?0c ?14 ?30 ? *	cfmvdhrcc	mvd12, ?r1
00000234 <move\+0x54> 1e ?08 ?24 ?30 ? *	cfmvdhrne	mvd8, ?r2
00000238 <move\+0x58> de ?06 ?c4 ?30 ? *	cfmvdhrle	mvd6, ?r12
0000023c <move\+0x5c> 4e ?02 ?34 ?30 ? *	cfmvdhrmi	mvd2, ?r3
00000240 <move\+0x60> 0e ?05 ?d4 ?30 ? *	cfmvdhreq	mvd5, ?sp
00000244 <move\+0x64> ae ?14 ?44 ?30 ? *	cfmvrdhge	r4, ?mvd4
00000248 <move\+0x68> ee ?18 ?b4 ?30 ? *	cfmvrdh	r11, ?mvd8
0000024c <move\+0x6c> de ?12 ?54 ?30 ? *	cfmvrdhle	r5, ?mvd2
00000250 <move\+0x70> 1e ?16 ?64 ?30 ? *	cfmvrdhne	r6, ?mvd6
00000254 <move\+0x74> be ?17 ?04 ?30 ? *	cfmvrdhlt	r0, ?mvd7
00000258 <move\+0x78> 5e ?0e ?45 ?10 ? *	cfmv64lrpl	mvdx14, ?r4
0000025c <move\+0x7c> ce ?0a ?b5 ?10 ? *	cfmv64lrgt	mvdx10, ?r11
00000260 <move\+0x80> 8e ?0f ?55 ?10 ? *	cfmv64lrhi	mvdx15, ?r5
00000264 <move\+0x84> 6e ?0b ?65 ?10 ? *	cfmv64lrvs	mvdx11, ?r6
00000268 <move\+0x88> 2e ?09 ?05 ?10 ? *	cfmv64lrcs	mvdx9, ?r0
0000026c <move\+0x8c> 5e ?1a ?d5 ?10 ? *	cfmvr64lpl	sp, ?mvdx10
00000270 <move\+0x90> 9e ?1e ?e5 ?10 ? *	cfmvr64lls	lr, ?mvdx14
00000274 <move\+0x94> 3e ?1d ?85 ?10 ? *	cfmvr64lcc	r8, ?mvdx13
00000278 <move\+0x98> 7e ?11 ?f5 ?10 ? *	cfmvr64lvc	pc, ?mvdx1
0000027c <move\+0x9c> ce ?1b ?95 ?10 ? *	cfmvr64lgt	r9, ?mvdx11
00000280 <move\+0xa0> 0e ?07 ?d5 ?30 ? *	cfmv64hreq	mvdx7, ?sp
00000284 <move\+0xa4> ee ?03 ?e5 ?30 ? *	cfmv64hr	mvdx3, ?lr
00000288 <move\+0xa8> ae ?01 ?85 ?30 ? *	cfmv64hrge	mvdx1, ?r8
0000028c <move\+0xac> ee ?0d ?f5 ?30 ? *	cfmv64hr	mvdx13, ?pc
00000290 <move\+0xb0> be ?04 ?95 ?30 ? *	cfmv64hrlt	mvdx4, ?r9
00000294 <move\+0xb4> 9e ?15 ?05 ?30 ? *	cfmvr64hls	r0, ?mvdx5
00000298 <move\+0xb8> ee ?19 ?75 ?30 ? *	cfmvr64h	r7, ?mvdx9
0000029c <move\+0xbc> 4e ?13 ?15 ?30 ? *	cfmvr64hmi	r1, ?mvdx3
000002a0 <move\+0xc0> 8e ?17 ?25 ?30 ? *	cfmvr64hhi	r2, ?mvdx7
000002a4 <move\+0xc4> 2e ?1c ?c5 ?30 ? *	cfmvr64hcs	r12, ?mvdx12
000002a8 <move\+0xc8> 6e ?10 ?06 ?11 ? *	cfmval32vs	mvax1, ?mvfx0
000002ac <move\+0xcc> 7e ?1e ?06 ?13 ? *	cfmval32vc	mvax3, ?mvfx14
000002b0 <move\+0xd0> 3e ?1a ?06 ?10 ? *	cfmval32cc	mvax0, ?mvfx10
000002b4 <move\+0xd4> 1e ?1f ?06 ?11 ? *	cfmval32ne	mvax1, ?mvfx15
000002b8 <move\+0xd8> de ?1b ?06 ?10 ? *	cfmval32le	mvax0, ?mvfx11
000002bc <move\+0xdc> 4e ?01 ?06 ?12 ? *	cfmv32almi	mvfx2, ?mvax1
000002c0 <move\+0xe0> 0e ?03 ?06 ?15 ? *	cfmv32aleq	mvfx5, ?mvax3
000002c4 <move\+0xe4> ae ?00 ?06 ?19 ? *	cfmv32alge	mvfx9, ?mvax0
000002c8 <move\+0xe8> ee ?01 ?06 ?13 ? *	cfmv32al	mvfx3, ?mvax1
000002cc <move\+0xec> de ?00 ?06 ?17 ? *	cfmv32alle	mvfx7, ?mvax0
000002d0 <move\+0xf0> 1e ?16 ?06 ?32 ? *	cfmvam32ne	mvax2, ?mvfx6
000002d4 <move\+0xf4> be ?17 ?06 ?30 ? *	cfmvam32lt	mvax0, ?mvfx7
000002d8 <move\+0xf8> 5e ?13 ?06 ?32 ? *	cfmvam32pl	mvax2, ?mvfx3
000002dc <move\+0xfc> ce ?11 ?06 ?31 ? *	cfmvam32gt	mvax1, ?mvfx1
000002e0 <move\+0x100> 8e ?1d ?06 ?33 ? *	cfmvam32hi	mvax3, ?mvfx13
000002e4 <move\+0x104> 6e ?02 ?06 ?3b ? *	cfmv32amvs	mvfx11, ?mvax2
000002e8 <move\+0x108> 2e ?00 ?06 ?39 ? *	cfmv32amcs	mvfx9, ?mvax0
000002ec <move\+0x10c> 5e ?02 ?06 ?3f ? *	cfmv32ampl	mvfx15, ?mvax2
000002f0 <move\+0x110> 9e ?01 ?06 ?34 ? *	cfmv32amls	mvfx4, ?mvax1
000002f4 <move\+0x114> 3e ?03 ?06 ?38 ? *	cfmv32amcc	mvfx8, ?mvax3
000002f8 <move\+0x118> 7e ?11 ?06 ?50 ? *	cfmvah32vc	mvax0, ?mvfx1
000002fc <move\+0x11c> ce ?1b ?06 ?50 ? *	cfmvah32gt	mvax0, ?mvfx11
00000300 <move\+0x120> 0e ?15 ?06 ?51 ? *	cfmvah32eq	mvax1, ?mvfx5
00000304 <move\+0x124> ee ?1c ?06 ?52 ? *	cfmvah32	mvax2, ?mvfx12
00000308 <move\+0x128> ae ?18 ?06 ?53 ? *	cfmvah32ge	mvax3, ?mvfx8
0000030c <move\+0x12c> ee ?00 ?06 ?5d ? *	cfmv32ah	mvfx13, ?mvax0
00000310 <move\+0x130> be ?00 ?06 ?54 ? *	cfmv32ahlt	mvfx4, ?mvax0
00000314 <move\+0x134> 9e ?01 ?06 ?50 ? *	cfmv32ahls	mvfx0, ?mvax1
00000318 <move\+0x138> ee ?02 ?06 ?5a ? *	cfmv32ah	mvfx10, ?mvax2
0000031c <move\+0x13c> 4e ?03 ?06 ?5e ? *	cfmv32ahmi	mvfx14, ?mvax3
00000320 <move\+0x140> 8e ?17 ?06 ?73 ? *	cfmva32hi	mvax3, ?mvfx7
00000324 <move\+0x144> 2e ?1c ?06 ?73 ? *	cfmva32cs	mvax3, ?mvfx12
00000328 <move\+0x148> 6e ?10 ?06 ?71 ? *	cfmva32vs	mvax1, ?mvfx0
0000032c <move\+0x14c> 7e ?1e ?06 ?73 ? *	cfmva32vc	mvax3, ?mvfx14
00000330 <move\+0x150> 3e ?1a ?06 ?70 ? *	cfmva32cc	mvax0, ?mvfx10
00000334 <move\+0x154> 1e ?03 ?06 ?78 ? *	cfmv32ane	mvfx8, ?mvax3
00000338 <move\+0x158> de ?03 ?06 ?76 ? *	cfmv32ale	mvfx6, ?mvax3
0000033c <move\+0x15c> 4e ?01 ?06 ?72 ? *	cfmv32ami	mvfx2, ?mvax1
00000340 <move\+0x160> 0e ?03 ?06 ?75 ? *	cfmv32aeq	mvfx5, ?mvax3
00000344 <move\+0x164> ae ?00 ?06 ?79 ? *	cfmv32age	mvfx9, ?mvax0
00000348 <move\+0x168> ee ?18 ?06 ?93 ? *	cfmva64	mvax3, ?mvdx8
0000034c <move\+0x16c> de ?12 ?06 ?92 ? *	cfmva64le	mvax2, ?mvdx2
00000350 <move\+0x170> 1e ?16 ?06 ?92 ? *	cfmva64ne	mvax2, ?mvdx6
00000354 <move\+0x174> be ?17 ?06 ?90 ? *	cfmva64lt	mvax0, ?mvdx7
00000358 <move\+0x178> 5e ?13 ?06 ?92 ? *	cfmva64pl	mvax2, ?mvdx3
0000035c <move\+0x17c> ce ?03 ?06 ?9a ? *	cfmv64agt	mvdx10, ?mvax3
00000360 <move\+0x180> 8e ?02 ?06 ?9f ? *	cfmv64ahi	mvdx15, ?mvax2
00000364 <move\+0x184> 6e ?02 ?06 ?9b ? *	cfmv64avs	mvdx11, ?mvax2
00000368 <move\+0x188> 2e ?00 ?06 ?99 ? *	cfmv64acs	mvdx9, ?mvax0
0000036c <move\+0x18c> 5e ?02 ?06 ?9f ? *	cfmv64apl	mvdx15, ?mvax2
00000370 <move\+0x190> 9e ?1e ?06 ?b0 ? *	cfmvsc32ls	dspsc, ?mvfx14
00000374 <move\+0x194> 3e ?1d ?06 ?b0 ? *	cfmvsc32cc	dspsc, ?mvfx13
00000378 <move\+0x198> 7e ?11 ?06 ?b0 ? *	cfmvsc32vc	dspsc, ?mvfx1
0000037c <move\+0x19c> ce ?1b ?06 ?b0 ? *	cfmvsc32gt	dspsc, ?mvfx11
00000380 <move\+0x1a0> 0e ?15 ?06 ?b0 ? *	cfmvsc32eq	dspsc, ?mvfx5
00000384 <move\+0x1a4> ee ?00 ?06 ?b3 ? *	cfmv32sc	mvfx3, ?dspsc
00000388 <move\+0x1a8> ae ?00 ?06 ?b1 ? *	cfmv32scge	mvfx1, ?dspsc
0000038c <move\+0x1ac> ee ?00 ?06 ?bd ? *	cfmv32sc	mvfx13, ?dspsc
00000390 <move\+0x1b0> be ?00 ?06 ?b4 ? *	cfmv32sclt	mvfx4, ?dspsc
00000394 <move\+0x1b4> 9e ?00 ?06 ?b0 ? *	cfmv32scls	mvfx0, ?dspsc
00000398 <move\+0x1b8> ee ?09 ?a4 ?00 ? *	cfcpys	mvf10, ?mvf9
0000039c <move\+0x1bc> 4e ?03 ?e4 ?00 ? *	cfcpysmi	mvf14, ?mvf3
000003a0 <move\+0x1c0> 8e ?07 ?d4 ?00 ? *	cfcpyshi	mvf13, ?mvf7
000003a4 <move\+0x1c4> 2e ?0c ?14 ?00 ? *	cfcpyscs	mvf1, ?mvf12
000003a8 <move\+0x1c8> 6e ?00 ?b4 ?00 ? *	cfcpysvs	mvf11, ?mvf0
000003ac <move\+0x1cc> 7e ?0e ?54 ?20 ? *	cfcpydvc	mvd5, ?mvd14
000003b0 <move\+0x1d0> 3e ?0a ?c4 ?20 ? *	cfcpydcc	mvd12, ?mvd10
000003b4 <move\+0x1d4> 1e ?0f ?84 ?20 ? *	cfcpydne	mvd8, ?mvd15
000003b8 <move\+0x1d8> de ?0b ?64 ?20 ? *	cfcpydle	mvd6, ?mvd11
000003bc <move\+0x1dc> 4e ?09 ?24 ?20 ? *	cfcpydmi	mvd2, ?mvd9
# conv:
000003c0 <conv> 0e ?0f ?54 ?60 ? *	cfcvtsdeq	mvd5, ?mvf15
000003c4 <conv\+0x4> ae ?04 ?94 ?60 ? *	cfcvtsdge	mvd9, ?mvf4
000003c8 <conv\+0x8> ee ?08 ?34 ?60 ? *	cfcvtsd	mvd3, ?mvf8
000003cc <conv\+0xc> de ?02 ?74 ?60 ? *	cfcvtsdle	mvd7, ?mvf2
000003d0 <conv\+0x10> 1e ?06 ?c4 ?60 ? *	cfcvtsdne	mvd12, ?mvf6
000003d4 <conv\+0x14> be ?07 ?04 ?40 ? *	cfcvtdslt	mvf0, ?mvd7
000003d8 <conv\+0x18> 5e ?03 ?e4 ?40 ? *	cfcvtdspl	mvf14, ?mvd3
000003dc <conv\+0x1c> ce ?01 ?a4 ?40 ? *	cfcvtdsgt	mvf10, ?mvd1
000003e0 <conv\+0x20> 8e ?0d ?f4 ?40 ? *	cfcvtdshi	mvf15, ?mvd13
000003e4 <conv\+0x24> 6e ?04 ?b4 ?40 ? *	cfcvtdsvs	mvf11, ?mvd4
000003e8 <conv\+0x28> 2e ?00 ?94 ?80 ? *	cfcvt32scs	mvf9, ?mvfx0
000003ec <conv\+0x2c> 5e ?0a ?f4 ?80 ? *	cfcvt32spl	mvf15, ?mvfx10
000003f0 <conv\+0x30> 9e ?0e ?44 ?80 ? *	cfcvt32sls	mvf4, ?mvfx14
000003f4 <conv\+0x34> 3e ?0d ?84 ?80 ? *	cfcvt32scc	mvf8, ?mvfx13
000003f8 <conv\+0x38> 7e ?01 ?24 ?80 ? *	cfcvt32svc	mvf2, ?mvfx1
000003fc <conv\+0x3c> ce ?0b ?64 ?a0 ? *	cfcvt32dgt	mvd6, ?mvfx11
00000400 <conv\+0x40> 0e ?05 ?74 ?a0 ? *	cfcvt32deq	mvd7, ?mvfx5
00000404 <conv\+0x44> ee ?0c ?34 ?a0 ? *	cfcvt32d	mvd3, ?mvfx12
00000408 <conv\+0x48> ae ?08 ?14 ?a0 ? *	cfcvt32dge	mvd1, ?mvfx8
0000040c <conv\+0x4c> ee ?06 ?d4 ?a0 ? *	cfcvt32d	mvd13, ?mvfx6
00000410 <conv\+0x50> be ?02 ?44 ?c0 ? *	cfcvt64slt	mvf4, ?mvdx2
00000414 <conv\+0x54> 9e ?05 ?04 ?c0 ? *	cfcvt64sls	mvf0, ?mvdx5
00000418 <conv\+0x58> ee ?09 ?a4 ?c0 ? *	cfcvt64s	mvf10, ?mvdx9
0000041c <conv\+0x5c> 4e ?03 ?e4 ?c0 ? *	cfcvt64smi	mvf14, ?mvdx3
00000420 <conv\+0x60> 8e ?07 ?d4 ?c0 ? *	cfcvt64shi	mvf13, ?mvdx7
00000424 <conv\+0x64> 2e ?0c ?14 ?e0 ? *	cfcvt64dcs	mvd1, ?mvdx12
00000428 <conv\+0x68> 6e ?00 ?b4 ?e0 ? *	cfcvt64dvs	mvd11, ?mvdx0
0000042c <conv\+0x6c> 7e ?0e ?54 ?e0 ? *	cfcvt64dvc	mvd5, ?mvdx14
00000430 <conv\+0x70> 3e ?0a ?c4 ?e0 ? *	cfcvt64dcc	mvd12, ?mvdx10
00000434 <conv\+0x74> 1e ?0f ?84 ?e0 ? *	cfcvt64dne	mvd8, ?mvdx15
00000438 <conv\+0x78> de ?1b ?65 ?80 ? *	cfcvts32le	mvfx6, ?mvf11
0000043c <conv\+0x7c> 4e ?19 ?25 ?80 ? *	cfcvts32mi	mvfx2, ?mvf9
00000440 <conv\+0x80> 0e ?1f ?55 ?80 ? *	cfcvts32eq	mvfx5, ?mvf15
00000444 <conv\+0x84> ae ?14 ?95 ?80 ? *	cfcvts32ge	mvfx9, ?mvf4
00000448 <conv\+0x88> ee ?18 ?35 ?80 ? *	cfcvts32	mvfx3, ?mvf8
0000044c <conv\+0x8c> de ?12 ?75 ?a0 ? *	cfcvtd32le	mvfx7, ?mvd2
00000450 <conv\+0x90> 1e ?16 ?c5 ?a0 ? *	cfcvtd32ne	mvfx12, ?mvd6
00000454 <conv\+0x94> be ?17 ?05 ?a0 ? *	cfcvtd32lt	mvfx0, ?mvd7
00000458 <conv\+0x98> 5e ?13 ?e5 ?a0 ? *	cfcvtd32pl	mvfx14, ?mvd3
0000045c <conv\+0x9c> ce ?11 ?a5 ?a0 ? *	cfcvtd32gt	mvfx10, ?mvd1
00000460 <conv\+0xa0> 8e ?1d ?f5 ?c0 ? *	cftruncs32hi	mvfx15, ?mvf13
00000464 <conv\+0xa4> 6e ?14 ?b5 ?c0 ? *	cftruncs32vs	mvfx11, ?mvf4
00000468 <conv\+0xa8> 2e ?10 ?95 ?c0 ? *	cftruncs32cs	mvfx9, ?mvf0
0000046c <conv\+0xac> 5e ?1a ?f5 ?c0 ? *	cftruncs32pl	mvfx15, ?mvf10
00000470 <conv\+0xb0> 9e ?1e ?45 ?c0 ? *	cftruncs32ls	mvfx4, ?mvf14
00000474 <conv\+0xb4> 3e ?1d ?85 ?e0 ? *	cftruncd32cc	mvfx8, ?mvd13
00000478 <conv\+0xb8> 7e ?11 ?25 ?e0 ? *	cftruncd32vc	mvfx2, ?mvd1
0000047c <conv\+0xbc> ce ?1b ?65 ?e0 ? *	cftruncd32gt	mvfx6, ?mvd11
00000480 <conv\+0xc0> 0e ?15 ?75 ?e0 ? *	cftruncd32eq	mvfx7, ?mvd5
00000484 <conv\+0xc4> ee ?1c ?35 ?e0 ? *	cftruncd32	mvfx3, ?mvd12
# shift:
00000488 <shift> ae ?01 ?25 ?58 ? *	cfrshl32ge	mvfx1, ?mvfx8, ?r2
0000048c <shift\+0x4> 6e ?0b ?95 ?54 ? *	cfrshl32vs	mvfx11, ?mvfx4, ?r9
00000490 <shift\+0x8> 0e ?05 ?75 ?5f ? *	cfrshl32eq	mvfx5, ?mvfx15, ?r7
00000494 <shift\+0xc> 4e ?0e ?85 ?53 ? *	cfrshl32mi	mvfx14, ?mvfx3, ?r8
00000498 <shift\+0x10> 7e ?02 ?65 ?51 ? *	cfrshl32vc	mvfx2, ?mvfx1, ?r6
0000049c <shift\+0x14> be ?00 ?d5 ?77 ? *	cfrshl64lt	mvdx0, ?mvdx7, ?sp
000004a0 <shift\+0x18> 3e ?0c ?b5 ?7a ? *	cfrshl64cc	mvdx12, ?mvdx10, ?r11
000004a4 <shift\+0x1c> ee ?0d ?c5 ?76 ? *	cfrshl64	mvdx13, ?mvdx6, ?r12
000004a8 <shift\+0x20> 2e ?09 ?a5 ?70 ? *	cfrshl64cs	mvdx9, ?mvdx0, ?r10
000004ac <shift\+0x24> ae ?09 ?15 ?74 ? *	cfrshl64ge	mvdx9, ?mvdx4, ?r1
000004b0 <shift\+0x28> 8e ?07 ?d5 ?41 ? *	cfsh32hi	mvfx13, ?mvfx7, ?#33
000004b4 <shift\+0x2c> ce ?0b ?65 ?00 ? *	cfsh32gt	mvfx6, ?mvfx11, ?#0
000004b8 <shift\+0x30> 5e ?03 ?e5 ?40 ? *	cfsh32pl	mvfx14, ?mvfx3, ?#32
000004bc <shift\+0x34> 1e ?0f ?85 ?c1 ? *	cfsh32ne	mvfx8, ?mvfx15, ?#-31
000004c0 <shift\+0x38> be ?02 ?45 ?01 ? *	cfsh32lt	mvfx4, ?mvfx2, ?#1
000004c4 <shift\+0x3c> 5e ?2a ?f5 ?c0 ? *	cfsh64pl	mvdx15, ?mvdx10, ?#-32
000004c8 <shift\+0x40> ee ?28 ?35 ?c5 ? *	cfsh64	mvdx3, ?mvdx8, ?#-27
000004cc <shift\+0x44> 2e ?2c ?15 ?eb ? *	cfsh64cs	mvdx1, ?mvdx12, ?#-5
000004d0 <shift\+0x48> 0e ?25 ?75 ?6f ? *	cfsh64eq	mvdx7, ?mvdx5, ?#63
000004d4 <shift\+0x4c> ce ?21 ?a5 ?09 ? *	cfsh64gt	mvdx10, ?mvdx1, ?#9
# comp:
000004d8 <comp> de ?1b ?f4 ?94 ? *	cfcmpsle	pc, ?mvf11, ?mvf4
000004dc <comp\+0x4> 9e ?15 ?04 ?9f ? *	cfcmpsls	r0, ?mvf5, ?mvf15
000004e0 <comp\+0x8> 9e ?1e ?e4 ?93 ? *	cfcmpsls	lr, ?mvf14, ?mvf3
000004e4 <comp\+0xc> de ?12 ?54 ?91 ? *	cfcmpsle	r5, ?mvf2, ?mvf1
000004e8 <comp\+0x10> 6e ?10 ?34 ?97 ? *	cfcmpsvs	r3, ?mvf0, ?mvf7
000004ec <comp\+0x14> ee ?1c ?44 ?ba ? *	cfcmpd	r4, ?mvd12, ?mvd10
000004f0 <comp\+0x18> 8e ?1d ?24 ?b6 ? *	cfcmpdhi	r2, ?mvd13, ?mvd6
000004f4 <comp\+0x1c> 4e ?19 ?94 ?b0 ? *	cfcmpdmi	r9, ?mvd9, ?mvd0
000004f8 <comp\+0x20> ee ?19 ?74 ?b4 ? *	cfcmpd	r7, ?mvd9, ?mvd4
000004fc <comp\+0x24> 3e ?1d ?84 ?b7 ? *	cfcmpdcc	r8, ?mvd13, ?mvd7
00000500 <comp\+0x28> 1e ?16 ?65 ?9b ? *	cfcmp32ne	r6, ?mvfx6, ?mvfx11
00000504 <comp\+0x2c> 7e ?1e ?d5 ?93 ? *	cfcmp32vc	sp, ?mvfx14, ?mvfx3
00000508 <comp\+0x30> ae ?18 ?b5 ?9f ? *	cfcmp32ge	r11, ?mvfx8, ?mvfx15
0000050c <comp\+0x34> 6e ?14 ?c5 ?92 ? *	cfcmp32vs	r12, ?mvfx4, ?mvfx2
00000510 <comp\+0x38> 0e ?1f ?a5 ?9a ? *	cfcmp32eq	r10, ?mvfx15, ?mvfx10
00000514 <comp\+0x3c> 4e ?13 ?15 ?b8 ? *	cfcmp64mi	r1, ?mvdx3, ?mvdx8
00000518 <comp\+0x40> 7e ?11 ?f5 ?bc ? *	cfcmp64vc	pc, ?mvdx1, ?mvdx12
0000051c <comp\+0x44> be ?17 ?05 ?b5 ? *	cfcmp64lt	r0, ?mvdx7, ?mvdx5
00000520 <comp\+0x48> 3e ?1a ?e5 ?b1 ? *	cfcmp64cc	lr, ?mvdx10, ?mvdx1
00000524 <comp\+0x4c> ee ?16 ?55 ?bb ? *	cfcmp64	r5, ?mvdx6, ?mvdx11
# fp_arith:
00000528 <fp_arith> 2e ?30 ?94 ?00 ? *	cfabsscs	mvf9, ?mvf0
0000052c <fp_arith\+0x4> 5e ?3a ?f4 ?00 ? *	cfabsspl	mvf15, ?mvf10
00000530 <fp_arith\+0x8> 9e ?3e ?44 ?00 ? *	cfabssls	mvf4, ?mvf14
00000534 <fp_arith\+0xc> 3e ?3d ?84 ?00 ? *	cfabsscc	mvf8, ?mvf13
00000538 <fp_arith\+0x10> 7e ?31 ?24 ?00 ? *	cfabssvc	mvf2, ?mvf1
0000053c <fp_arith\+0x14> ce ?3b ?64 ?20 ? *	cfabsdgt	mvd6, ?mvd11
00000540 <fp_arith\+0x18> 0e ?35 ?74 ?20 ? *	cfabsdeq	mvd7, ?mvd5
00000544 <fp_arith\+0x1c> ee ?3c ?34 ?20 ? *	cfabsd	mvd3, ?mvd12
00000548 <fp_arith\+0x20> ae ?38 ?14 ?20 ? *	cfabsdge	mvd1, ?mvd8
0000054c <fp_arith\+0x24> ee ?36 ?d4 ?20 ? *	cfabsd	mvd13, ?mvd6
00000550 <fp_arith\+0x28> be ?32 ?44 ?40 ? *	cfnegslt	mvf4, ?mvf2
00000554 <fp_arith\+0x2c> 9e ?35 ?04 ?40 ? *	cfnegsls	mvf0, ?mvf5
00000558 <fp_arith\+0x30> ee ?39 ?a4 ?40 ? *	cfnegs	mvf10, ?mvf9
0000055c <fp_arith\+0x34> 4e ?33 ?e4 ?40 ? *	cfnegsmi	mvf14, ?mvf3
00000560 <fp_arith\+0x38> 8e ?37 ?d4 ?40 ? *	cfnegshi	mvf13, ?mvf7
00000564 <fp_arith\+0x3c> 2e ?3c ?14 ?60 ? *	cfnegdcs	mvd1, ?mvd12
00000568 <fp_arith\+0x40> 6e ?30 ?b4 ?60 ? *	cfnegdvs	mvd11, ?mvd0
0000056c <fp_arith\+0x44> 7e ?3e ?54 ?60 ? *	cfnegdvc	mvd5, ?mvd14
00000570 <fp_arith\+0x48> 3e ?3a ?c4 ?60 ? *	cfnegdcc	mvd12, ?mvd10
00000574 <fp_arith\+0x4c> 1e ?3f ?84 ?60 ? *	cfnegdne	mvd8, ?mvd15
00000578 <fp_arith\+0x50> de ?3b ?64 ?84 ? *	cfaddsle	mvf6, ?mvf11, ?mvf4
0000057c <fp_arith\+0x54> 9e ?35 ?04 ?8f ? *	cfaddsls	mvf0, ?mvf5, ?mvf15
00000580 <fp_arith\+0x58> 9e ?3e ?44 ?83 ? *	cfaddsls	mvf4, ?mvf14, ?mvf3
00000584 <fp_arith\+0x5c> de ?32 ?74 ?81 ? *	cfaddsle	mvf7, ?mvf2, ?mvf1
00000588 <fp_arith\+0x60> 6e ?30 ?b4 ?87 ? *	cfaddsvs	mvf11, ?mvf0, ?mvf7
0000058c <fp_arith\+0x64> ee ?3c ?34 ?aa ? *	cfaddd	mvd3, ?mvd12, ?mvd10
00000590 <fp_arith\+0x68> 8e ?3d ?f4 ?a6 ? *	cfadddhi	mvd15, ?mvd13, ?mvd6
00000594 <fp_arith\+0x6c> 4e ?39 ?24 ?a0 ? *	cfadddmi	mvd2, ?mvd9, ?mvd0
00000598 <fp_arith\+0x70> ee ?39 ?a4 ?a4 ? *	cfaddd	mvd10, ?mvd9, ?mvd4
0000059c <fp_arith\+0x74> 3e ?3d ?84 ?a7 ? *	cfadddcc	mvd8, ?mvd13, ?mvd7
000005a0 <fp_arith\+0x78> 1e ?36 ?c4 ?cb ? *	cfsubsne	mvf12, ?mvf6, ?mvf11
000005a4 <fp_arith\+0x7c> 7e ?3e ?54 ?c3 ? *	cfsubsvc	mvf5, ?mvf14, ?mvf3
000005a8 <fp_arith\+0x80> ae ?38 ?14 ?cf ? *	cfsubsge	mvf1, ?mvf8, ?mvf15
000005ac <fp_arith\+0x84> 6e ?34 ?b4 ?c2 ? *	cfsubsvs	mvf11, ?mvf4, ?mvf2
000005b0 <fp_arith\+0x88> 0e ?3f ?54 ?ca ? *	cfsubseq	mvf5, ?mvf15, ?mvf10
000005b4 <fp_arith\+0x8c> 4e ?33 ?e4 ?e8 ? *	cfsubdmi	mvd14, ?mvd3, ?mvd8
000005b8 <fp_arith\+0x90> 7e ?31 ?24 ?ec ? *	cfsubdvc	mvd2, ?mvd1, ?mvd12
000005bc <fp_arith\+0x94> be ?37 ?04 ?e5 ? *	cfsubdlt	mvd0, ?mvd7, ?mvd5
000005c0 <fp_arith\+0x98> 3e ?3a ?c4 ?e1 ? *	cfsubdcc	mvd12, ?mvd10, ?mvd1
000005c4 <fp_arith\+0x9c> ee ?36 ?d4 ?eb ? *	cfsubd	mvd13, ?mvd6, ?mvd11
000005c8 <fp_arith\+0xa0> 2e ?10 ?94 ?05 ? *	cfmulscs	mvf9, ?mvf0, ?mvf5
000005cc <fp_arith\+0xa4> ae ?14 ?94 ?0e ? *	cfmulsge	mvf9, ?mvf4, ?mvf14
000005d0 <fp_arith\+0xa8> 8e ?17 ?d4 ?02 ? *	cfmulshi	mvf13, ?mvf7, ?mvf2
000005d4 <fp_arith\+0xac> ce ?1b ?64 ?00 ? *	cfmulsgt	mvf6, ?mvf11, ?mvf0
000005d8 <fp_arith\+0xb0> 5e ?13 ?e4 ?0c ? *	cfmulspl	mvf14, ?mvf3, ?mvf12
000005dc <fp_arith\+0xb4> 1e ?1f ?84 ?2d ? *	cfmuldne	mvd8, ?mvd15, ?mvd13
000005e0 <fp_arith\+0xb8> be ?12 ?44 ?29 ? *	cfmuldlt	mvd4, ?mvd2, ?mvd9
000005e4 <fp_arith\+0xbc> 5e ?1a ?f4 ?29 ? *	cfmuldpl	mvd15, ?mvd10, ?mvd9
000005e8 <fp_arith\+0xc0> ee ?18 ?34 ?2d ? *	cfmuld	mvd3, ?mvd8, ?mvd13
000005ec <fp_arith\+0xc4> 2e ?1c ?14 ?26 ? *	cfmuldcs	mvd1, ?mvd12, ?mvd6
# int_arith:
000005f0 <int_arith> 0e ?35 ?75 ?00 ? *	cfabs32eq	mvfx7, ?mvfx5
000005f4 <int_arith\+0x4> ee ?3c ?35 ?00 ? *	cfabs32	mvfx3, ?mvfx12
000005f8 <int_arith\+0x8> ae ?38 ?15 ?00 ? *	cfabs32ge	mvfx1, ?mvfx8
000005fc <int_arith\+0xc> ee ?36 ?d5 ?00 ? *	cfabs32	mvfx13, ?mvfx6
00000600 <int_arith\+0x10> be ?32 ?45 ?00 ? *	cfabs32lt	mvfx4, ?mvfx2
00000604 <int_arith\+0x14> 9e ?35 ?05 ?20 ? *	cfabs64ls	mvdx0, ?mvdx5
00000608 <int_arith\+0x18> ee ?39 ?a5 ?20 ? *	cfabs64	mvdx10, ?mvdx9
0000060c <int_arith\+0x1c> 4e ?33 ?e5 ?20 ? *	cfabs64mi	mvdx14, ?mvdx3
00000610 <int_arith\+0x20> 8e ?37 ?d5 ?20 ? *	cfabs64hi	mvdx13, ?mvdx7
00000614 <int_arith\+0x24> 2e ?3c ?15 ?20 ? *	cfabs64cs	mvdx1, ?mvdx12
00000618 <int_arith\+0x28> 6e ?30 ?b5 ?40 ? *	cfneg32vs	mvfx11, ?mvfx0
0000061c <int_arith\+0x2c> 7e ?3e ?55 ?40 ? *	cfneg32vc	mvfx5, ?mvfx14
00000620 <int_arith\+0x30> 3e ?3a ?c5 ?40 ? *	cfneg32cc	mvfx12, ?mvfx10
00000624 <int_arith\+0x34> 1e ?3f ?85 ?40 ? *	cfneg32ne	mvfx8, ?mvfx15
00000628 <int_arith\+0x38> de ?3b ?65 ?40 ? *	cfneg32le	mvfx6, ?mvfx11
0000062c <int_arith\+0x3c> 4e ?39 ?25 ?60 ? *	cfneg64mi	mvdx2, ?mvdx9
00000630 <int_arith\+0x40> 0e ?3f ?55 ?60 ? *	cfneg64eq	mvdx5, ?mvdx15
00000634 <int_arith\+0x44> ae ?34 ?95 ?60 ? *	cfneg64ge	mvdx9, ?mvdx4
00000638 <int_arith\+0x48> ee ?38 ?35 ?60 ? *	cfneg64	mvdx3, ?mvdx8
0000063c <int_arith\+0x4c> de ?32 ?75 ?60 ? *	cfneg64le	mvdx7, ?mvdx2
00000640 <int_arith\+0x50> 1e ?36 ?c5 ?8b ? *	cfadd32ne	mvfx12, ?mvfx6, ?mvfx11
00000644 <int_arith\+0x54> 7e ?3e ?55 ?83 ? *	cfadd32vc	mvfx5, ?mvfx14, ?mvfx3
00000648 <int_arith\+0x58> ae ?38 ?15 ?8f ? *	cfadd32ge	mvfx1, ?mvfx8, ?mvfx15
0000064c <int_arith\+0x5c> 6e ?34 ?b5 ?82 ? *	cfadd32vs	mvfx11, ?mvfx4, ?mvfx2
00000650 <int_arith\+0x60> 0e ?3f ?55 ?8a ? *	cfadd32eq	mvfx5, ?mvfx15, ?mvfx10
00000654 <int_arith\+0x64> 4e ?33 ?e5 ?a8 ? *	cfadd64mi	mvdx14, ?mvdx3, ?mvdx8
00000658 <int_arith\+0x68> 7e ?31 ?25 ?ac ? *	cfadd64vc	mvdx2, ?mvdx1, ?mvdx12
0000065c <int_arith\+0x6c> be ?37 ?05 ?a5 ? *	cfadd64lt	mvdx0, ?mvdx7, ?mvdx5
00000660 <int_arith\+0x70> 3e ?3a ?c5 ?a1 ? *	cfadd64cc	mvdx12, ?mvdx10, ?mvdx1
00000664 <int_arith\+0x74> ee ?36 ?d5 ?ab ? *	cfadd64	mvdx13, ?mvdx6, ?mvdx11
00000668 <int_arith\+0x78> 2e ?30 ?95 ?c5 ? *	cfsub32cs	mvfx9, ?mvfx0, ?mvfx5
0000066c <int_arith\+0x7c> ae ?34 ?95 ?ce ? *	cfsub32ge	mvfx9, ?mvfx4, ?mvfx14
00000670 <int_arith\+0x80> 8e ?37 ?d5 ?c2 ? *	cfsub32hi	mvfx13, ?mvfx7, ?mvfx2
00000674 <int_arith\+0x84> ce ?3b ?65 ?c0 ? *	cfsub32gt	mvfx6, ?mvfx11, ?mvfx0
00000678 <int_arith\+0x88> 5e ?33 ?e5 ?cc ? *	cfsub32pl	mvfx14, ?mvfx3, ?mvfx12
0000067c <int_arith\+0x8c> 1e ?3f ?85 ?ed ? *	cfsub64ne	mvdx8, ?mvdx15, ?mvdx13
00000680 <int_arith\+0x90> be ?32 ?45 ?e9 ? *	cfsub64lt	mvdx4, ?mvdx2, ?mvdx9
00000684 <int_arith\+0x94> 5e ?3a ?f5 ?e9 ? *	cfsub64pl	mvdx15, ?mvdx10, ?mvdx9
00000688 <int_arith\+0x98> ee ?38 ?35 ?ed ? *	cfsub64	mvdx3, ?mvdx8, ?mvdx13
0000068c <int_arith\+0x9c> 2e ?3c ?15 ?e6 ? *	cfsub64cs	mvdx1, ?mvdx12, ?mvdx6
00000690 <int_arith\+0xa0> 0e ?15 ?75 ?0e ? *	cfmul32eq	mvfx7, ?mvfx5, ?mvfx14
00000694 <int_arith\+0xa4> ce ?11 ?a5 ?08 ? *	cfmul32gt	mvfx10, ?mvfx1, ?mvfx8
00000698 <int_arith\+0xa8> de ?1b ?65 ?04 ? *	cfmul32le	mvfx6, ?mvfx11, ?mvfx4
0000069c <int_arith\+0xac> 9e ?15 ?05 ?0f ? *	cfmul32ls	mvfx0, ?mvfx5, ?mvfx15
000006a0 <int_arith\+0xb0> 9e ?1e ?45 ?03 ? *	cfmul32ls	mvfx4, ?mvfx14, ?mvfx3
000006a4 <int_arith\+0xb4> de ?12 ?75 ?21 ? *	cfmul64le	mvdx7, ?mvdx2, ?mvdx1
000006a8 <int_arith\+0xb8> 6e ?10 ?b5 ?27 ? *	cfmul64vs	mvdx11, ?mvdx0, ?mvdx7
000006ac <int_arith\+0xbc> ee ?1c ?35 ?2a ? *	cfmul64	mvdx3, ?mvdx12, ?mvdx10
000006b0 <int_arith\+0xc0> 8e ?1d ?f5 ?26 ? *	cfmul64hi	mvdx15, ?mvdx13, ?mvdx6
000006b4 <int_arith\+0xc4> 4e ?19 ?25 ?20 ? *	cfmul64mi	mvdx2, ?mvdx9, ?mvdx0
000006b8 <int_arith\+0xc8> ee ?19 ?a5 ?44 ? *	cfmac32	mvfx10, ?mvfx9, ?mvfx4
000006bc <int_arith\+0xcc> 3e ?1d ?85 ?47 ? *	cfmac32cc	mvfx8, ?mvfx13, ?mvfx7
000006c0 <int_arith\+0xd0> 1e ?16 ?c5 ?4b ? *	cfmac32ne	mvfx12, ?mvfx6, ?mvfx11
000006c4 <int_arith\+0xd4> 7e ?1e ?55 ?43 ? *	cfmac32vc	mvfx5, ?mvfx14, ?mvfx3
000006c8 <int_arith\+0xd8> ae ?18 ?15 ?4f ? *	cfmac32ge	mvfx1, ?mvfx8, ?mvfx15
000006cc <int_arith\+0xdc> 6e ?14 ?b5 ?62 ? *	cfmsc32vs	mvfx11, ?mvfx4, ?mvfx2
000006d0 <int_arith\+0xe0> 0e ?1f ?55 ?6a ? *	cfmsc32eq	mvfx5, ?mvfx15, ?mvfx10
000006d4 <int_arith\+0xe4> 4e ?13 ?e5 ?68 ? *	cfmsc32mi	mvfx14, ?mvfx3, ?mvfx8
000006d8 <int_arith\+0xe8> 7e ?11 ?25 ?6c ? *	cfmsc32vc	mvfx2, ?mvfx1, ?mvfx12
000006dc <int_arith\+0xec> be ?17 ?05 ?65 ? *	cfmsc32lt	mvfx0, ?mvfx7, ?mvfx5
# acc_arith:
000006e0 <acc_arith> 3e ?01 ?a6 ?08 ? *	cfmadd32cc	mvax0, ?mvfx10, ?mvfx1, ?mvfx8
000006e4 <acc_arith\+0x4> ee ?0b ?66 ?44 ? *	cfmadd32	mvax2, ?mvfx6, ?mvfx11, ?mvfx4
000006e8 <acc_arith\+0x8> 2e ?05 ?06 ?2f ? *	cfmadd32cs	mvax1, ?mvfx0, ?mvfx5, ?mvfx15
000006ec <acc_arith\+0xc> ae ?0e ?46 ?43 ? *	cfmadd32ge	mvax2, ?mvfx4, ?mvfx14, ?mvfx3
000006f0 <acc_arith\+0x10> 8e ?02 ?76 ?61 ? *	cfmadd32hi	mvax3, ?mvfx7, ?mvfx2, ?mvfx1
000006f4 <acc_arith\+0x14> ce ?10 ?b6 ?07 ? *	cfmsub32gt	mvax0, ?mvfx11, ?mvfx0, ?mvfx7
000006f8 <acc_arith\+0x18> 5e ?1c ?36 ?4a ? *	cfmsub32pl	mvax2, ?mvfx3, ?mvfx12, ?mvfx10
000006fc <acc_arith\+0x1c> 1e ?1d ?f6 ?26 ? *	cfmsub32ne	mvax1, ?mvfx15, ?mvfx13, ?mvfx6
00000700 <acc_arith\+0x20> be ?19 ?26 ?40 ? *	cfmsub32lt	mvax2, ?mvfx2, ?mvfx9, ?mvfx0
00000704 <acc_arith\+0x24> 5e ?19 ?a6 ?64 ? *	cfmsub32pl	mvax3, ?mvfx10, ?mvfx9, ?mvfx4
00000708 <acc_arith\+0x28> ee ?2d ?16 ?67 ? *	cfmadda32	mvax3, ?mvax1, ?mvfx13, ?mvfx7
0000070c <acc_arith\+0x2c> 2e ?26 ?26 ?6b ? *	cfmadda32cs	mvax3, ?mvax2, ?mvfx6, ?mvfx11
00000710 <acc_arith\+0x30> 0e ?2e ?36 ?23 ? *	cfmadda32eq	mvax1, ?mvax3, ?mvfx14, ?mvfx3
00000714 <acc_arith\+0x34> ce ?28 ?36 ?2f ? *	cfmadda32gt	mvax1, ?mvax3, ?mvfx8, ?mvfx15
00000718 <acc_arith\+0x38> de ?24 ?36 ?02 ? *	cfmadda32le	mvax0, ?mvax3, ?mvfx4, ?mvfx2
0000071c <acc_arith\+0x3c> 9e ?3f ?16 ?0a ? *	cfmsuba32ls	mvax0, ?mvax1, ?mvfx15, ?mvfx10
00000720 <acc_arith\+0x40> 9e ?33 ?16 ?08 ? *	cfmsuba32ls	mvax0, ?mvax1, ?mvfx3, ?mvfx8
00000724 <acc_arith\+0x44> de ?31 ?06 ?4c ? *	cfmsuba32le	mvax2, ?mvax0, ?mvfx1, ?mvfx12
00000728 <acc_arith\+0x48> 6e ?37 ?06 ?25 ? *	cfmsuba32vs	mvax1, ?mvax0, ?mvfx7, ?mvfx5
0000072c <acc_arith\+0x4c> ee ?3a ?06 ?41 ? *	cfmsuba32	mvax2, ?mvax0, ?mvfx10, ?mvfx1
