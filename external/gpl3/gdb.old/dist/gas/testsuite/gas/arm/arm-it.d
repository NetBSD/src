#name: ARM IT instruction
#objdump: -dr --prefix-addresses --show-raw-insn
#skip: *-*-pe *-*-wince

.*: +file format .*arm.*

Disassembly of section .text:
0+000 <[^>]*> 03a00000 ?	moveq	r0, #0
0+004 <[^>]*> e1a0f00e ?	mov	pc, lr
