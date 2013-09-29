.macro foo arg, rest:vararg
	\rest
.endm
	foo r0, pld [r0]
