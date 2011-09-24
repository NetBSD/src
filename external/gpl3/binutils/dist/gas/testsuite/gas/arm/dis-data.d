# name: Data disassembler test (no symbols)
# skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*
# objdump: -dr --prefix-addresses --show-raw-insn

.*: +file format .*arm.*

Disassembly of section \.text:
0x00000000 20010000 	andcs	r0, r1, r0
0x00000004 000000f9 	strdeq	r0, \[r0\], -r9
0x00000008 00004cd5 	ldrdeq	r4, \[r0\], -r5
