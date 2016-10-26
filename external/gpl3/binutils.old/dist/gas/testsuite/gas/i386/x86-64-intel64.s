# Check Intel64

	.text
	.arch core2
_start:
	syscall
	sysret
