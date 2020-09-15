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
	iretq
	sysretl
	sysretq

	.intel_syntax noprefix
	iretw
	iretd
	iret
	iretq
	sysretd
	sysretq
