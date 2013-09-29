#objdump: -dr
#name: TLS
# This test is only valid on ELF based ports.
#not-target: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd *-*-riscix*
# VxWorks needs a special variant of this file.
#skip: *-*-vxworks*

# Test generation of TLS relocations

.*: +file format .*arm.*

Disassembly of section .text:

0+00 <arm_fn>:
   0:	e1a00000 	nop			; .*
			0: R_ARM_TLS_DESCSEQ	af
   4:	e59f0014 	ldr	r0, \[pc, #20\]	; 20 .*
   8:	fa000000 	blx	8 <ae\+.*>
			8: R_ARM_TLS_CALL	ae
   c:	e1a00000 	nop			; .*
0+10 <.arm_pool>:
  10:	00000008 	.word	0x00000008
			10: R_ARM_TLS_GD32	aa
  14:	0000000c 	.word	0x0000000c
			14: R_ARM_TLS_LDM32	ab
  18:	00000010 	.word	0x00000010
			18: R_ARM_TLS_IE32	ac
  1c:	00000000 	.word	0x00000000
			1c: R_ARM_TLS_LE32	ad
  20:	00000018 	.word	0x00000018
			20: R_ARM_TLS_GOTDESC	ae
0+24 <thumb_fn>:
  24:	46c0      	nop			; .*
  26:	46c0      	nop			; .*
			26: R_ARM_THM_TLS_DESCSEQ	tf
  28:	4805      	ldr	r0, \[pc, #20\]	; \(40 .*\)
  2a:	f000 e800 	blx	4 <te\+0x4>
			2a: R_ARM_THM_TLS_CALL	te
  2e:	46c0      	nop			; .*
  30:	00000002 	.word	0x00000002
			30: R_ARM_TLS_GD32	ta
  34:	00000006 	.word	0x00000006
			34: R_ARM_TLS_LDM32	tb
  38:	0000000a 	.word	0x0000000a
			38: R_ARM_TLS_IE32	tc
  3c:	00000000 	.word	0x00000000
			3c: R_ARM_TLS_LE32	td
  40:	00000017 	.word	0x00000017
			40: R_ARM_TLS_GOTDESC	te
