# name: Data disassembler test (function symbol)
# skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*
# objdump: -dr --prefix-addresses --show-raw-insn

.*: +file format .*arm.*

Disassembly of section \.text:
00000000 <main> 20010000 	andcs	r0, r1, r0
00000004 <main\+0x4> 000000f9 	strdeq	r0, \[r0\], -r9
00000008 <main\+0x8> 00004cd5 	ldrdeq	r4, \[r0\], -r5
