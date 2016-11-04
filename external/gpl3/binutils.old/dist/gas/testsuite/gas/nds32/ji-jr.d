#objdump: -dr --prefix-addresses
#name: nds32 load-store instructions
#as:

# Test ls instructions

.*:     file format .*

Disassembly of section .text:
0+0000 <[^>]*> j8	00000000 <foo>
			0: R_NDS32_9_PCREL_RELA	.text
			0: R_NDS32_RELAX_ENTRY	.text
0+0002 <[^>]*> jal	00000002 <foo\+0x2>
			2: R_NDS32_25_PCREL_RELA	.text
0+0006 <[^>]*> jr	\$r0
0+000a <[^>]*> jral	\$lp, \$r0
0+000e <[^>]*> ret	\$lp
