#objdump: -dr --prefix-addresses --show-raw-insn
#name: ADRL

# Test the `ADRL' pseudo-op

.*: +file format .*arm.*

Disassembly of section .text:
	...
00002000 <.*> e24f0008 	sub	r0, pc, #8	; 0x8
00002004 <.*> e2400c20 	sub	r0, r0, #8192	; 0x2000
00002008 <.*> e28f0018 	add	r0, pc, #24	; 0x18
0000200c <.*> e2800c20 	add	r0, r0, #8192	; 0x2000
00002010 <.*> e24f0018 	sub	r0, pc, #24	; 0x18
00002014 <.*> e1a00000 	nop			\(mov r0,r0\)
00002018 <.*> e28f0000 	add	r0, pc, #0	; 0x0
0000201c <.*> e1a00000 	nop			\(mov r0,r0\)
	...
