#as: -mcpu=arc700
#objdump: -dr --show-raw-insn

.*: +file format .*arc.*

Disassembly of section .text:

[0-9a-f]+ <text_label>:
   0:	2020 0f80 0000 0000 	j	0
			4: ARC_32_ME	text_label
   8:	20e0 0f80 0000 0000 	j	0
			c: ARC_32_ME	text_label
  10:	20e0 0f80 0000 0000 	j	0
			14: ARC_32_ME	text_label
  18:	20e0 0f81 0000 0000 	jeq	0
			1c: ARC_32_ME	text_label
  20:	20e0 0f81 0000 0000 	jeq	0
			24: ARC_32_ME	text_label
  28:	20e0 0f82 0000 0000 	jne	0
			2c: ARC_32_ME	text_label
  30:	20e0 0f82 0000 0000 	jne	0
			34: ARC_32_ME	text_label
  38:	20e0 0f83 0000 0000 	jp	0
			3c: ARC_32_ME	text_label
  40:	20e0 0f83 0000 0000 	jp	0
			44: ARC_32_ME	text_label
  48:	20e0 0f84 0000 0000 	jn	0
			4c: ARC_32_ME	text_label
  50:	20e0 0f84 0000 0000 	jn	0
			54: ARC_32_ME	text_label
  58:	20e0 0f85 0000 0000 	jc	0
			5c: ARC_32_ME	text_label
  60:	20e0 0f85 0000 0000 	jc	0
			64: ARC_32_ME	text_label
  68:	20e0 0f85 0000 0000 	jc	0
			6c: ARC_32_ME	text_label
  70:	20e0 0f86 0000 0000 	jnc	0
			74: ARC_32_ME	text_label
  78:	20e0 0f86 0000 0000 	jnc	0
			7c: ARC_32_ME	text_label
  80:	20e0 0f86 0000 0000 	jnc	0
			84: ARC_32_ME	text_label
  88:	20e0 0f87 0000 0000 	jv	0
			8c: ARC_32_ME	text_label
  90:	20e0 0f87 0000 0000 	jv	0
			94: ARC_32_ME	text_label
  98:	20e0 0f88 0000 0000 	jnv	0
			9c: ARC_32_ME	text_label
  a0:	20e0 0f88 0000 0000 	jnv	0
			a4: ARC_32_ME	text_label
  a8:	20e0 0f89 0000 0000 	jgt	0
			ac: ARC_32_ME	text_label
  b0:	20e0 0f8a 0000 0000 	jge	0
			b4: ARC_32_ME	text_label
  b8:	20e0 0f8b 0000 0000 	jlt	0
			bc: ARC_32_ME	text_label
  c0:	20e0 0f8c 0000 0000 	jle	0
			c4: ARC_32_ME	text_label
  c8:	20e0 0f8d 0000 0000 	jhi	0
			cc: ARC_32_ME	text_label
  d0:	20e0 0f8e 0000 0000 	jls	0
			d4: ARC_32_ME	text_label
  d8:	20e0 0f8f 0000 0000 	jpnz	0
			dc: ARC_32_ME	text_label
  e0:	2020 0f80 0000 0000 	j	0
			e4: ARC_32_ME	external_text_label
  e8:	20a0 0000           	j	0
