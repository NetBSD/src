	/* ARMv8.1 System instructions.  */
	.text

	/* RAS Extension.  */
	esb
	hint #0x10

	/* Statistical profiling.  */
	psb csync
	hint #0x11
