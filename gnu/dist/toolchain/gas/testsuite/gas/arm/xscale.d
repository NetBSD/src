#objdump: -dr --prefix-addresses --show-raw-insn
#name: XScale instructions
#as: -mxscale -EL

# Test the XScale instructions:

.*: +file format .*arm.*

Disassembly of section .text:
00000000 <foo> ee201010 	mia	acc0, r0, r1
00000004 <[^>]*> be20d01e 	mialt	acc0, lr, sp
00000008 <[^>]*> ee284012 	miaph	acc0, r2, r4
0000000c <[^>]*> 1e286015 	miaphne	acc0, r5, r6
00000010 <[^>]*> ee2c8017 	miaBB	acc0, r7, r8
00000014 <[^>]*> ee2da019 	miaBT	acc0, r9, sl
00000018 <[^>]*> ee2eb01c 	miaTB	acc0, ip, fp
0000001c <[^>]*> ee2f0010 	miaTT	acc0, r0, r0
00000020 <[^>]*> ec411000 	mar	acc0, r1, r1
00000024 <[^>]*> cc4c2000 	margt	acc0, r2, ip
00000028 <[^>]*> ec543000 	mra	r3, r4, acc0
0000002c <[^>]*> ec585000 	mra	r5, r8, acc0
00000030 <[^>]*> f5d0f000 	pld	\[r0\]
00000034 <[^>]*> f5d1f789 	pld	\[r1, #1929\]
00000038 <[^>]*> f7d2f003 	pld	\[r2, r3\]
0000003c <[^>]*> f754f285 	pld	\[r4, -r5, lsl #5\]
00000040 <[^>]*> f456f456 	pld	\[r6\], -#1110
00000044 <[^>]*> f6d7f008 	pld	\[r7\], r8
00000048 <[^>]*> f659f06a 	pld	\[r9\], -sl, rrx
0000004c <[^>]*> e1c100d0 	ldrd	r0, \[r1\]
00000050 <[^>]*> 01c327d8 	ldreqd	r2, \[r3, #120\]
00000054 <[^>]*> b10540d6 	ldrltd	r4, \[r5, -r6\]
00000058 <[^>]*> e16a88f9 	strd	r8, \[sl, -#137\]!
0000005c <[^>]*> e1ac00fd 	strd	r0, \[ip, sp\]!
00000060 <[^>]*> 30ce21f0 	strccd	r2, \[lr\], #16
00000064 <[^>]*> 708640f8 	strvcd	r4, \[r6\], r8
00000068 <[^>]*> e5910000 	ldr	r0, \[r1\]
0000006c <[^>]*> e5832000 	str	r2, \[r3\]
00000070 <[^>]*> e321f011 	msr	CPSR_c, #17	; 0x11
