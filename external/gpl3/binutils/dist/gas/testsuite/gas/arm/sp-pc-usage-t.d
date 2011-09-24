# name: SP and PC registers special uses test.
# objdump: -d --prefix-addresses --show-raw-insn

.*: +file format .*arm.*

Disassembly of section .text:
00000000 <foo> 4685      	mov	sp, r0
00000002 <foo\+0x2> 4668      	mov	r0, sp
00000004 <foo\+0x4> b000      	add	sp, #0
00000006 <foo\+0x6> f20d 0d00 	addw	sp, sp, #0
0000000a <foo\+0xa> b080      	sub	sp, #0
0000000c <foo\+0xc> f2ad 0d00 	subw	sp, sp, #0
00000010 <foo\+0x10> 4485      	add	sp, r0
00000012 <foo\+0x12> eb0d 0d40 	add.w	sp, sp, r0, lsl #1
00000016 <foo\+0x16> ebad 0d00 	sub.w	sp, sp, r0
0000001a <foo\+0x1a> ebad 0d40 	sub.w	sp, sp, r0, lsl #1
0000001e <foo\+0x1e> 9800      	ldr	r0, \[sp, #0\]
00000020 <foo\+0x20> 4800      	ldr	r0, \[pc, #0\]	; \(00000024 <foo\+0x24>\)
00000022 <foo\+0x22> f8d0 f000 	ldr.w	pc, \[r0\]
00000026 <foo\+0x26> f8d0 d000 	ldr.w	sp, \[r0\]
0000002a <foo\+0x2a> f8df f000 	ldr.w	pc, \[pc\]	; 0000002c <foo\+0x2c>
0000002e <foo\+0x2e> f8dd d000 	ldr.w	sp, \[sp\]
00000032 <foo\+0x32> f8dd f000 	ldr.w	pc, \[sp\]
00000036 <foo\+0x36> f8df d000 	ldr.w	sp, \[pc\]	; 00000038 <foo\+0x38>
0000003a <foo\+0x3a> f850 d00f 	ldr.w	sp, \[r0, pc\]
0000003e <foo\+0x3e> 9000      	str	r0, \[sp, #0\]
00000040 <foo\+0x40> f8c0 d000 	str.w	sp, \[r0\]
00000044 <foo\+0x44> f8cd d000 	str.w	sp, \[sp\]
00000048 <foo\+0x48> f840 d00f 	str.w	sp, \[r0, pc\]
0000004c <foo\+0x4c> 4468      	add	r0, sp
0000004e <foo\+0x4e> eb1d 0000 	adds.w	r0, sp, r0
00000052 <foo\+0x52> eb0d 0040 	add.w	r0, sp, r0, lsl #1
00000056 <foo\+0x56> eb1d 0040 	adds.w	r0, sp, r0, lsl #1
0000005a <foo\+0x5a> f11d 0f00 	cmn.w	sp, #0
0000005e <foo\+0x5e> eb1d 0f00 	cmn.w	sp, r0
00000062 <foo\+0x62> eb1d 0f40 	cmn.w	sp, r0, lsl #1
00000066 <foo\+0x66> f1bd 0f00 	cmp.w	sp, #0
0000006a <foo\+0x6a> 4585      	cmp	sp, r0
0000006c <foo\+0x6c> ebbd 0f40 	cmp.w	sp, r0, lsl #1
00000070 <foo\+0x70> b080      	sub	sp, #0
00000072 <foo\+0x72> f1bd 0d00 	subs.w	sp, sp, #0
00000076 <foo\+0x76> f1ad 0000 	sub.w	r0, sp, #0
0000007a <foo\+0x7a> f1bd 0000 	subs.w	r0, sp, #0
0000007e <foo\+0x7e> b001      	add	sp, #4
00000080 <foo\+0x80> a801      	add	r0, sp, #4
00000082 <foo\+0x82> f11d 0d04 	adds.w	sp, sp, #4
00000086 <foo\+0x86> f11d 0004 	adds.w	r0, sp, #4
0000008a <foo\+0x8a> f20d 0004 	addw	r0, sp, #4
0000008e <foo\+0x8e> b001      	add	sp, #4
00000090 <foo\+0x90> f11d 0d04 	adds.w	sp, sp, #4
00000094 <foo\+0x94> f20d 0d04 	addw	sp, sp, #4
00000098 <foo\+0x98> 4485      	add	sp, r0
0000009a <foo\+0x9a> 4468      	add	r0, sp
0000009c <foo\+0x9c> eb0d 0040 	add.w	r0, sp, r0, lsl #1
000000a0 <foo\+0xa0> eb1d 0d00 	adds.w	sp, sp, r0
000000a4 <foo\+0xa4> eb1d 0000 	adds.w	r0, sp, r0
000000a8 <foo\+0xa8> eb1d 0040 	adds.w	r0, sp, r0, lsl #1
000000ac <foo\+0xac> 4485      	add	sp, r0
000000ae <foo\+0xae> eb0d 0d40 	add.w	sp, sp, r0, lsl #1
000000b2 <foo\+0xb2> eb1d 0d00 	adds.w	sp, sp, r0
000000b6 <foo\+0xb6> eb1d 0d40 	adds.w	sp, sp, r0, lsl #1
000000ba <foo\+0xba> 44ed      	add	sp, sp
000000bc <foo\+0xbc> f1ad 0000 	sub.w	r0, sp, #0
000000c0 <foo\+0xc0> f1bd 0000 	subs.w	r0, sp, #0
000000c4 <foo\+0xc4> f2ad 0000 	subw	r0, sp, #0
000000c8 <foo\+0xc8> b080      	sub	sp, #0
000000ca <foo\+0xca> f1bd 0d00 	subs.w	sp, sp, #0
000000ce <foo\+0xce> f2ad 0d00 	subw	sp, sp, #0
000000d2 <foo\+0xd2> b080      	sub	sp, #0
000000d4 <foo\+0xd4> f1bd 0d00 	subs.w	sp, sp, #0
000000d8 <foo\+0xd8> ebad 0040 	sub.w	r0, sp, r0, lsl #1
000000dc <foo\+0xdc> ebbd 0040 	subs.w	r0, sp, r0, lsl #1
000000e0 <foo\+0xe0> ebad 0d40 	sub.w	sp, sp, r0, lsl #1
000000e4 <foo\+0xe4> ebbd 0d40 	subs.w	sp, sp, r0, lsl #1
000000e8 <foo\+0xe8> a001      	add	r0, pc, #4	; \(adr r0, 000000f0 <foo\+0xf0>\)
000000ea <foo\+0xea> f2af 0004 	subw	r0, pc, #4
000000ee <foo\+0xee> f20f 0004 	addw	r0, pc, #4
000000f2 <foo\+0xf2> f2af 0004 	subw	r0, pc, #4
000000f6 <foo\+0xf6> f20f 0004 	addw	r0, pc, #4
000000fa <foo\+0xfa> f2af 0004 	subw	r0, pc, #4
000000fe <foo\+0xfe> bf00[ 	]+nop
00000100 <foo\+0x100> bf00[ 	]+nop
00000102 <foo\+0x102> bf00[ 	]+nop
