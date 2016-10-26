# Disassembling with -Msuffix.

	.text
foo:
	monitor
	mwait

	vmcall
	vmlaunch
	vmresume
	vmxoff

	iretw
	iretl
	iret
	iretq
	sysretl
	sysret
	sysretq

	.intel_syntax noprefix
	iretw
	iretd
	iret
	iretq
	sysretd
	sysret
	sysretq
