#objdump: -dr --prefix-addresses --show-raw-insn
#name: MIPS bge
#source: bge.s
#as: -32

# Test the bge macro (microMIPS).

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> 00a4 0b50 	slt	at,a0,a1
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+0004 <text_label\+0x4>
			4: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 4044 fffe 	bgez	a0,0+000a <text_label\+0xa>
			a: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 4085 fffe 	blez	a1,0+0010 <text_label\+0x10>
			10: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 4044 fffe 	bgez	a0,0+0016 <text_label\+0x16>
			16: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 40c4 fffe 	bgtz	a0,0+001c <text_label\+0x1c>
			1c: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9024 0002 	slti	at,a0,2
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+0026 <text_label\+0x26>
			26: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 5020 8000 	li	at,0x8000
[0-9a-f]+ <[^>]*> 0024 0b50 	slt	at,a0,at
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+0034 <text_label\+0x34>
			34: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9024 8000 	slti	at,a0,-32768
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+003e <text_label\+0x3e>
			3e: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 41a1 0001 	lui	at,0x1
[0-9a-f]+ <[^>]*> 0024 0b50 	slt	at,a0,at
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+004c <text_label\+0x4c>
			4c: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 41a1 0001 	lui	at,0x1
[0-9a-f]+ <[^>]*> 5021 a5a5 	ori	at,at,0xa5a5
[0-9a-f]+ <[^>]*> 0024 0b50 	slt	at,a0,at
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+005e <text_label\+0x5e>
			5e: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 0085 0b50 	slt	at,a1,a0
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+0068 <text_label\+0x68>
			68: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 40c4 fffe 	bgtz	a0,0+006e <text_label\+0x6e>
			6e: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 4005 fffe 	bltz	a1,0+0074 <text_label\+0x74>
			74: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 40c4 fffe 	bgtz	a0,0+007a <text_label\+0x7a>
			7a: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 00a4 0b50 	slt	at,a0,a1
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+0084 <text_label\+0x84>
			84: R_MICROMIPS_PC16_S1	external_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 0085 0b50 	slt	at,a1,a0
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+008e <text_label\+0x8e>
			8e: R_MICROMIPS_PC16_S1	external_label
[0-9a-f]+ <[^>]*> 0c00      	nop
	\.\.\.
