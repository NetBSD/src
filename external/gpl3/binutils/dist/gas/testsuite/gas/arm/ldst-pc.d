#objdump: -dr --prefix-addresses --show-raw-insn
#name: ARM load/store with pc base register
#as: -mno-warn-deprecated

# Test the standard ARM instructions:

.*: +file format .*arm.*

Disassembly of section .text:
0+000 <[^>]*> e51f1008 	ldr	r1, \[pc, #-8\]	; 0+000 <[^>]*>
0+004 <[^>]*> e79f1002 	ldr	r1, \[pc, r2\]
0+008 <[^>]*> f55ff008 	pld	\[pc, #-8\]	; 0+008 <[^>]*>
0+00c <[^>]*> f7dff001 	pld	\[pc, r1\]
0+010 <[^>]*> f45ff008 	pli	\[pc, #-8\]	; 0+010 <[^>]*>
0+014 <[^>]*> f6dff001 	pli	\[pc, r1\]
0+018 <[^>]*> e58f1004 	str	r1, \[pc, #4\]	; 0+024 <[^>]*>
