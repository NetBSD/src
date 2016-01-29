#objdump: -dr --prefix-addresses --show-raw-insn
#name: ARMv8 CRC32 instructions
#as: -march=armv8-a+crc
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

.*: *file format .*arm.*


Disassembly of section .text:
0+0 <[^>]*> e1010042 	crc32b	r0, r1, r2
0+4 <[^>]*> e1210042 	crc32h	r0, r1, r2
0+8 <[^>]*> e1410042 	crc32w	r0, r1, r2
0+c <[^>]*> e1010242 	crc32cb	r0, r1, r2
0+10 <[^>]*> e1210242 	crc32ch	r0, r1, r2
0+14 <[^>]*> e1410242 	crc32cw	r0, r1, r2
0+18 <[^>]*> fac1 f082 	crc32b	r0, r1, r2
0+1c <[^>]*> fac1 f092 	crc32h	r0, r1, r2
0+20 <[^>]*> fac1 f0a2 	crc32w	r0, r1, r2
0+24 <[^>]*> fad1 f082 	crc32cb	r0, r1, r2
0+28 <[^>]*> fad1 f092 	crc32ch	r0, r1, r2
0+2c <[^>]*> fad1 f0a2 	crc32cw	r0, r1, r2

