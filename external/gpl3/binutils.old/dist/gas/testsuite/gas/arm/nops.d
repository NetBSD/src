# name: NOP<c> instructions
# objdump: -dr --prefix-addresses --show-raw-insn
# skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd

.*: +file format .*arm.*

Disassembly of section \.text:
0+000 <[^>]*> 0320f000 ?	nopeq	\{0\}
0+004 <[^>]*> 7320f000 ?	nopvc	\{0\}
0+008 <[^>]*> 7320d700 ?	nopvc	\{0\}	; <UNPREDICTABLE>


