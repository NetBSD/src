#objdump: -dr --prefix-addresses --show-raw-insn
#name: MIPS bltu
#source: bltu.s
#as: -32

# Test the bltu macro (microMIPS).

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> 00a4 0b90 	sltu	at,a0,a1
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+0004 <text_label\+0x4>
			4: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> b4a0 fffe 	bne	zero,a1,0+000a <text_label\+0xa>
			a: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9404 fffe 	beqz	a0,0+0010 <text_label\+0x10>
			10: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> b024 0002 	sltiu	at,a0,2
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+001a <text_label\+0x1a>
			1a: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 5020 8000 	li	at,0x8000
[0-9a-f]+ <[^>]*> 0024 0b90 	sltu	at,a0,at
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+0028 <text_label\+0x28>
			28: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> b024 8000 	sltiu	at,a0,-32768
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+0032 <text_label\+0x32>
			32: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 41a1 0001 	lui	at,0x1
[0-9a-f]+ <[^>]*> 0024 0b90 	sltu	at,a0,at
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+0040 <text_label\+0x40>
			40: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 41a1 0001 	lui	at,0x1
[0-9a-f]+ <[^>]*> 5021 a5a5 	ori	at,at,0xa5a5
[0-9a-f]+ <[^>]*> 0024 0b90 	sltu	at,a0,at
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+0052 <text_label\+0x52>
			52: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 0085 0b90 	sltu	at,a1,a0
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+005c <text_label\+0x5c>
			5c: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9404 fffe 	beqz	a0,0+0062 <text_label\+0x62>
			62: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9404 fffe 	beqz	a0,0+0068 <text_label\+0x68>
			68: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 00a4 0b90 	sltu	at,a0,a1
[0-9a-f]+ <[^>]*> b401 fffe 	bnez	at,0+0072 <text_label\+0x72>
			72: R_MICROMIPS_PC16_S1	external_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 0085 0b90 	sltu	at,a1,a0
[0-9a-f]+ <[^>]*> 9401 fffe 	beqz	at,0+007c <text_label\+0x7c>
			7c: R_MICROMIPS_PC16_S1	external_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 0c00      	nop
	\.\.\.
