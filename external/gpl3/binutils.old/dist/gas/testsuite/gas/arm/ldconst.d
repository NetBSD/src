#objdump: -dr --prefix-addresses --show-raw-insn
#name: ARM ldr with immediate constant
#as: -mcpu=arm7m -EL
# skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd

.*: +file format .*arm.*

Disassembly of section .text:
0+00 <[^>]*> e3a00000 ?	mov	r0, #0
0+04 <[^>]*> e3a004ff ?	mov	r0, #-16777216	; 0xff000000
0+08 <[^>]*> e3e00000 ?	mvn	r0, #0
0+0c <[^>]*> e51f0004 ?	ldr	r0, \[pc, #-4\]	; 0+10 <[^>]*>
0+10 <[^>]*> 0fff0000 ?	.*
0+14 <[^>]*> e3a0e000 ?	mov	lr, #0
0+18 <[^>]*> e3a0e8ff ?	mov	lr, #16711680	; 0xff0000
0+1c <[^>]*> e3e0e8ff ?	mvn	lr, #16711680	; 0xff0000
0+20 <[^>]*> e51fe004 ?	ldr	lr, \[pc, #-4\]	; 0+24 <[^>]*>
0+24 <[^>]*> 00fff000 ?	.*
0+28 <[^>]*> 03a00000 ?	moveq	r0, #0
0+2c <[^>]*> 03a00cff ?	moveq	r0, #65280	; 0xff00
0+30 <[^>]*> 03e00cff ?	mvneq	r0, #65280	; 0xff00
0+34 <[^>]*> 051f0004 ?	ldreq	r0, \[pc, #-4\]	; 0+38 <[^>]*>
0+38 <[^>]*> 000fff00 ?	.*
0+3c <[^>]*> 43a0b000 ?	movmi	fp, #0
0+40 <[^>]*> 43a0b0ff ?	movmi	fp, #255	; 0xff
0+44 <[^>]*> 43e0b0ff ?	mvnmi	fp, #255	; 0xff
0+48 <[^>]*> 451fb004 ?	ldrmi	fp, \[pc, #-4\]	; 0+4c <[^>]*>
0+4c <[^>]*> 0000fff0 ?	.*
0+50 <[^>]*> e59f0020 ?	ldr	r0, \[pc, #32\]	; 0+78 <[^>]*>
0+54 <[^>]*> e59f301c ?	ldr	r3, \[pc, #28\]	; 0+78 <[^>]*>
0+58 <[^>]*> e59f8018 ?	ldr	r8, \[pc, #24\]	; 0+78 <[^>]*>
0+5c <[^>]*> e59fb014 ?	ldr	fp, \[pc, #20\]	; 0+78 <[^>]*>
0+60 <[^>]*> e59fe010 ?	ldr	lr, \[pc, #16\]	; 0+78 <[^>]*>
0+64 <[^>]*> e59f0010 ?	ldr	r0, \[pc, #16\]	; 0+7c <[^>]*>
0+68 <[^>]*> e59f300c ?	ldr	r3, \[pc, #12\]	; 0+7c <[^>]*>
0+6c <[^>]*> e59f8008 ?	ldr	r8, \[pc, #8\]	; 0+7c <[^>]*>
0+70 <[^>]*> e59fb004 ?	ldr	fp, \[pc, #4\]	; 0+7c <[^>]*>
0+74 <[^>]*> e51fe000 ?	ldr	lr, \[pc, #-0\]	; 0+7c <[^>]*>
0+78 <[^>]*> 00000000 	.word	0x00000000
			78: R_ARM_ABS32	ext_symbol
0+7c <[^>]*> 00001000 	.word	0x00001000
			7c: R_ARM_ABS32	ext_symbol
