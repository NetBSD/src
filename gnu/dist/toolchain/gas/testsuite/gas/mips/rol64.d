#objdump: -dr --prefix-addresses -mmips:4000
#as: -march=r4000 -mtune=r4000
#name: MIPS R4000 drol

# Test the drol and dror macros.

.*: +file format .*mips.*

Disassembly of section .text:
0+0000 <[^>]*> dnegu	at,a1
0+0004 <[^>]*> dsrlv	at,a0,at
0+0008 <[^>]*> dsllv	a0,a0,a1
0+000c <[^>]*> or	a0,a0,at
0+0010 <[^>]*> dnegu	at,a2
0+0014 <[^>]*> dsrlv	at,a1,at
0+0018 <[^>]*> dsllv	a0,a1,a2
0+001c <[^>]*> or	a0,a0,at
0+0020 <[^>]*> dsll	at,a0,0x1
0+0024 <[^>]*> dsrl32	a0,a0,0x1f
0+0028 <[^>]*> or	a0,a0,at
0+002c <[^>]*> dsrl	a0,a1,0x0
0+0030 <[^>]*> dsll	at,a1,0x1
0+0034 <[^>]*> dsrl32	a0,a1,0x1f
0+0038 <[^>]*> or	a0,a0,at
0+003c <[^>]*> dsll	at,a1,0x1f
0+0040 <[^>]*> dsrl32	a0,a1,0x1
0+0044 <[^>]*> or	a0,a0,at
0+0048 <[^>]*> dsll32	at,a1,0x0
0+004c <[^>]*> dsrl32	a0,a1,0x0
0+0050 <[^>]*> or	a0,a0,at
0+0054 <[^>]*> dsll32	at,a1,0x1
0+0058 <[^>]*> dsrl	a0,a1,0x1f
0+005c <[^>]*> or	a0,a0,at
0+0060 <[^>]*> dsll32	at,a1,0x1f
0+0064 <[^>]*> dsrl	a0,a1,0x1
0+0068 <[^>]*> or	a0,a0,at
0+006c <[^>]*> dsrl	a0,a1,0x0
0+0070 <[^>]*> dnegu	at,a1
0+0074 <[^>]*> dsllv	at,a0,at
0+0078 <[^>]*> dsrlv	a0,a0,a1
0+007c <[^>]*> or	a0,a0,at
0+0080 <[^>]*> dnegu	at,a2
0+0084 <[^>]*> dsllv	at,a1,at
0+0088 <[^>]*> dsrlv	a0,a1,a2
0+008c <[^>]*> or	a0,a0,at
0+0090 <[^>]*> dsrl	at,a0,0x1
0+0094 <[^>]*> dsll32	a0,a0,0x1f
0+0098 <[^>]*> or	a0,a0,at
0+009c <[^>]*> dsrl	a0,a1,0x0
0+00a0 <[^>]*> dsrl	at,a1,0x1
0+00a4 <[^>]*> dsll32	a0,a1,0x1f
0+00a8 <[^>]*> or	a0,a0,at
0+00ac <[^>]*> dsrl	at,a1,0x1f
0+00b0 <[^>]*> dsll32	a0,a1,0x1
0+00b4 <[^>]*> or	a0,a0,at
0+00b8 <[^>]*> dsrl32	at,a1,0x0
0+00bc <[^>]*> dsll32	a0,a1,0x0
0+00c0 <[^>]*> or	a0,a0,at
0+00c4 <[^>]*> dsrl32	at,a1,0x1
0+00c8 <[^>]*> dsll	a0,a1,0x1f
0+00cc <[^>]*> or	a0,a0,at
0+00d0 <[^>]*> dsrl32	at,a1,0x1f
0+00d4 <[^>]*> dsll	a0,a1,0x1
0+00d8 <[^>]*> or	a0,a0,at
0+00dc <[^>]*> dsrl	a0,a1,0x0
	...
