#objdump: -dr --prefix-addresses -mmips:4000
#name: MIPS blt
#as: -32

# Test the blt macro.

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> slt	at,a0,a1
[0-9a-f]+ <[^>]*> bnez	at,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> bltz	a0,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> bgtz	a1,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> bltz	a0,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> blez	a0,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> slti	at,a0,2
[0-9a-f]+ <[^>]*> bnez	at,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> li	at,0x8000
[0-9a-f]+ <[^>]*> slt	at,a0,at
[0-9a-f]+ <[^>]*> bnez	at,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> slti	at,a0,-32768
[0-9a-f]+ <[^>]*> bnez	at,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> lui	at,0x1
[0-9a-f]+ <[^>]*> slt	at,a0,at
[0-9a-f]+ <[^>]*> bnez	at,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> lui	at,0x1
[0-9a-f]+ <[^>]*> ori	at,at,0xa5a5
[0-9a-f]+ <[^>]*> slt	at,a0,at
[0-9a-f]+ <[^>]*> bnez	at,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> slt	at,a1,a0
[0-9a-f]+ <[^>]*> beqz	at,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> blez	a0,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> bgez	a1,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> blez	a0,0+0000 <.*>
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> slt	at,a0,a1
[0-9a-f]+ <[^>]*> bnez	at,0+00a0 <.*\+0xa0>
[ 	]*a0: .*16	external_label
[0-9a-f]+ <[^>]*> nop
[0-9a-f]+ <[^>]*> slt	at,a1,a0
[0-9a-f]+ <[^>]*> beqz	at,0+00ac <.*\+0xac>
[ 	]*ac: .*16	external_label
[0-9a-f]+ <[^>]*> nop
	\.\.\.
