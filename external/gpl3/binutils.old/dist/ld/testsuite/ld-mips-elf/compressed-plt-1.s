	.macro	call_stub, name
	.set	push
	.set	nomips16
	.section .mips16.call.\name, "ax", @progbits
	.ent	__call_stub_\name
	.type	__call_stub_\name, @function
__call_stub_\name:
	la	$25, \name
	jr	$25
	.set	pop
	.endm

	# Flags to specify how a particular function is referenced

	.equ	DC, 1		# Direct call from "compressed" code
	.equ	IC, 2		# Indirect call from "compressed" code
	.equ	DU, 4		# Direct call from "uncompressed" code
	.equ	IU, 8		# Indirect call from "uncompressed" code
	.equ	LO, 16		# Direct address reference (%lo)

	# A wrapper around a macro called test_one, which is defined by
	# the file that includes this one.  NAME is the name of a function
	# that is referenced in the way described by FLAGS, an inclusive OR
	# of the flags above.  The wrapper filters out any functions whose
	# FLAGS are not a subset of FILTER.

	.macro	test_filter, name, flags
	.if	(\flags & filter) == \flags
	test_one \name, \flags
	.endif
	.endm

	.macro	test_all_dc, name, flags
	test_filter \name, \flags
	test_filter \name\()_dc, (\flags | DC)
	.endm

	.macro	test_all_ic, name, flags
	test_all_dc \name, \flags
	test_all_dc \name\()_ic, (\flags | IC)
	.endm

	.macro	test_all_du, name, flags
	test_all_ic \name, \flags
	test_all_ic \name\()_du, (\flags | DU)
	.endm

	.macro	test_all_iu, name, flags
	test_all_du \name, \flags
	test_all_du \name\()_iu, (\flags | IU)
	.endm

	.macro	test_all_lo, name, flags
	test_all_iu \name, \flags
	test_all_iu \name\()_lo, (\flags | LO)
	.endm

	# Test all the combinations of interest.

	.macro	test_all
	test_all_lo f, 0
	.endm
