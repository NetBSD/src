.macro try opcode:vararg
	.byte \opcode, 0x00
	.byte \opcode, 0x08
	.byte \opcode, 0x10
	.byte \opcode, 0x18
	.byte \opcode, 0x20
	.byte \opcode, 0x28
	.byte \opcode, 0x30
	.byte \opcode, 0x38
.endm

.text

amd_prefetch:
	try 0x0f, 0x0d

intel_prefetch:
	try 0x0f, 0x18
