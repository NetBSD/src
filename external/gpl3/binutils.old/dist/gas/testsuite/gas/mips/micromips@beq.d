#objdump: -dr --prefix-addresses --show-raw-insn
#name: MIPS beq
#source: beq.s
#as: -32

# Test the beq macro (microMIPS).

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> 94a4 fffe 	beq	a0,a1,0+0000 <text_label>
			0: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 9404 fffe 	beqz	a0,0+0006 <text_label\+0x6>
			6: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 3020 0001 	li	at,1
[0-9a-f]+ <[^>]*> 9424 fffe 	beq	a0,at,0+0010 <text_label\+0x10>
			10: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 5020 8000 	li	at,0x8000
[0-9a-f]+ <[^>]*> 9424 fffe 	beq	a0,at,0+001a <text_label\+0x1a>
			1a: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 3020 8000 	li	at,-32768
[0-9a-f]+ <[^>]*> 9424 fffe 	beq	a0,at,0+0024 <text_label\+0x24>
			24: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 41a1 0001 	lui	at,0x1
[0-9a-f]+ <[^>]*> 9424 fffe 	beq	a0,at,0+002e <text_label\+0x2e>
			2e: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 41a1 0001 	lui	at,0x1
[0-9a-f]+ <[^>]*> 5021 a5a5 	ori	at,at,0xa5a5
[0-9a-f]+ <[^>]*> 9424 fffe 	beq	a0,at,0+003c <text_label\+0x3c>
			3c: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> b404 fffe 	bnez	a0,0+0042 <text_label\+0x42>
			42: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
	\.\.\.
[0-9a-f]+ <[^>]*> 9400 fffe 	b	00020048 <text_label\+0x20048>
			20048: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0c00      	nop
[0-9a-f]+ <[^>]*> 4060 fffe 	bal	0002004e <text_label\+0x2004e>
			2004e: R_MICROMIPS_PC16_S1	text_label
[0-9a-f]+ <[^>]*> 0000 0000 	nop
[0-9a-f]+ <[^>]*> 0c00      	nop
	\.\.\.
