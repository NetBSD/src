# MACRO: start
# All assembler tests should start with a call to "start"
	.macro start
	.text

	# Skip over these inlined funcs.
	jmp __start;

	.global __pass
	.type __pass, function
__pass:
	write 1, _passmsg, 5
	exit 0

	.global __fail
	.type __fail, function
__fail:
	write 1, _failmsg, 5
	exit 1

	.data
_passmsg:
	.ascii "pass\n"
	.align 4

_failmsg:
	.ascii "fail\n"
	.align 4

	.text
	.global __start
	.type __start, function
__start:
	.endm

# MACRO: system_call
# Make a libgloss/Linux system call
	.macro system_call nr:req
	call #(0x180|\nr);
	.endm

# MACRO: exit
# Quit the current test
	.macro exit rc:req
	mov #\rc, r12
	system_call 1
	.endm

# MACRO: pass
# Write 'pass' to stdout via syscalls and quit;
# meant for non-OS operating environments
	.macro pass
	jmp __pass;
	.endm

# MACRO: fail
# Write 'fail' to stdout via syscalls and quit;
# meant for non-OS operating environments
	.macro fail
	jmp __fail;
	.endm

# MACRO: write
# Just like the write() C function; uses system calls
	.macro write fd:req, buf:req, count:req
	mov #\fd, r12;
	mov #\buf, r13;
	mov #\count, r14;
	system_call 5
	.endm
