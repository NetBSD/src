#objdump: -dr --prefix-addresses --show-raw-insn
#name: Unpredictable ARMv8 CRC32 instructions.
#as: -march=armv8-a+crc
#stderr: crc32-bad.l
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*

.*: +file format .*arm.*


Disassembly of section .text:
0+0 <[^>]*> e101f042 	crc32b	pc, r1, r2	; <UNPREDICTABLE>
0+4 <[^>]*> e12f0042 	crc32h	r0, pc, r2	; <UNPREDICTABLE>
0+8 <[^>]*> e141004f 	crc32w	r0, r1, pc	; <UNPREDICTABLE>
0+c <[^>]*> e10f0242 	crc32cb	r0, pc, r2	; <UNPREDICTABLE>
0+10 <[^>]*> e121f242 	crc32ch	pc, r1, r2	; <UNPREDICTABLE>
0+14 <[^>]*> e14f0242 	crc32cw	r0, pc, r2	; <UNPREDICTABLE>
0+18 <[^>]*> fac1 fd82 	crc32b	sp, r1, r2	; <UNPREDICTABLE>
0+1c <[^>]*> facf f092 	crc32h	r0, pc, r2	; <UNPREDICTABLE>
0+20 <[^>]*> fac1 f0ad 	crc32w	r0, r1, sp	; <UNPREDICTABLE>
0+24 <[^>]*> fadf f082 	crc32cb	r0, pc, r2	; <UNPREDICTABLE>
0+28 <[^>]*> fad1 fd92 	crc32ch	sp, r1, r2	; <UNPREDICTABLE>
0+2c <[^>]*> fadf f0a2 	crc32cw	r0, pc, r2	; <UNPREDICTABLE>
