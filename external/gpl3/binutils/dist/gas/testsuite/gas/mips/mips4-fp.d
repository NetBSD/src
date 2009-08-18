#objdump: -dr --prefix-addresses
#name: MIPS mips4 fp

# Test mips4 fp instructions.

.*: +file format .*mips.*

Disassembly of section .text:
0+0000 <[^>]*> bc1f	00000000+ <text_label>
0+0004 <[^>]*> nop
0+0008 <[^>]*> bc1f	\$fcc1,00000000+ <text_label>
0+000c <[^>]*> nop
0+0010 <[^>]*> bc1fl	\$fcc1,00000000+ <text_label>
0+0014 <[^>]*> nop
0+0018 <[^>]*> bc1t	\$fcc1,00000000+ <text_label>
0+001c <[^>]*> nop
0+0020 <[^>]*> bc1tl	\$fcc2,00000000+ <text_label>
0+0024 <[^>]*> nop
0+0028 <[^>]*> c.f.d	\$f4,\$f6
0+002c <[^>]*> c.f.d	\$fcc1,\$f4,\$f6
0+0030 <[^>]*> ldxc1	\$f2,a0\(a1\)
0+0034 <[^>]*> lwxc1	\$f2,a0\(a1\)
0+0038 <[^>]*> madd.d	\$f0,\$f2,\$f4,\$f6
0+003c <[^>]*> madd.s	\$f10,\$f8,\$f2,\$f0
0+0040 <[^>]*> movf	a0,a1,\$fcc4
0+0044 <[^>]*> movf.d	\$f4,\$f6,\$fcc0
0+0048 <[^>]*> movf.s	\$f4,\$f6,\$fcc0
0+004c <[^>]*> movn.d	\$f4,\$f6,a2
0+0050 <[^>]*> movn.s	\$f4,\$f6,a2
0+0054 <[^>]*> movt	a0,a1,\$fcc4
0+0058 <[^>]*> movt.d	\$f4,\$f6,\$fcc0
0+005c <[^>]*> movt.s	\$f4,\$f6,\$fcc0
0+0060 <[^>]*> movz.d	\$f4,\$f6,a2
0+0064 <[^>]*> movz.s	\$f4,\$f6,a2
0+0068 <[^>]*> msub.d	\$f0,\$f2,\$f4,\$f6
0+006c <[^>]*> msub.s	\$f0,\$f2,\$f4,\$f6
0+0070 <[^>]*> nmadd.d	\$f0,\$f2,\$f4,\$f6
0+0074 <[^>]*> nmadd.s	\$f0,\$f2,\$f4,\$f6
0+0078 <[^>]*> nmsub.d	\$f0,\$f2,\$f4,\$f6
0+007c <[^>]*> nmsub.s	\$f0,\$f2,\$f4,\$f6
0+0080 <[^>]*> prefx	0x4,a0\(a1\)
0+0084 <[^>]*> recip.d	\$f4,\$f6
0+0088 <[^>]*> recip.s	\$f4,\$f6
0+008c <[^>]*> rsqrt.d	\$f4,\$f6
0+0090 <[^>]*> rsqrt.s	\$f4,\$f6
0+0094 <[^>]*> sdxc1	\$f4,a0\(a1\)
0+0098 <[^>]*> swxc1	\$f4,a0\(a1\)
	...
