#name: Wide instruction acceptance in Thumb-2 cores
#objdump: -d --prefix-addresses --show-raw-insn
#skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

.*: +file format .*arm.*

Disassembly of section .text:
00000000 <.text> f7ff fffe 	bl	00000000 <foo>
00000004 <.text\+0x4> f3ef 8000 	mrs	r0, CPSR
