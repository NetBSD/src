.text
.align 0

label:
	# ARMV6K instructions
	clrex
	ldrexb r3, [r12]
	ldrexbne r12, [r3]
	ldrexd r3, [r12]
	ldrexdne r12, [r3]
	ldrexh r3, [r12]
	ldrexhne r12, [r3]
	nop {128}
	nopne {127}
	sev
	strexb r3, r12, [r7]
	strexbne r12, r3, [r8]
	strexd r3, r12, [r7]
	strexdne r12, r3, [r8]
	strexh r3, r12, [r7]
	strexhne r12, r3, [r8]
	wfe
	wfi
	yield
	# ARMV6Z instructions
	smi 0xec31
	smine 0x13ce

	# Add three nop instructions to ensure that the 
	# output is 32-byte aligned as required for arm-aout.
	nop
	nop
	nop
