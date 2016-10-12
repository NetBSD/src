# MACRO: exit
	.macro exit nr
	addi r3, r0, \nr;
	bri 0;
	.endm

# MACRO: pass
# Write 'pass' to stdout and quit
	.macro pass
	exit 0
	.data
	1: .asciz "pass\n"
	.endm

# MACRO: fail
# Write 'fail' to stdout and quit
	.macro fail
	exit 1
	.data
	1: .asciz "fail\n"
	.endm

# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.text
.global _start
_start:
	.endm
