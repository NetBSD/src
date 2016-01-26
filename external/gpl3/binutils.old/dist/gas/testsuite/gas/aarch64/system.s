	.text
	drps

	//
	// HINTS
	//

	nop
	yield
	wfe
	wfi
	sev
	sevl

	.macro	all_hints from=0, to=127
	hint \from
	.if	\to-\from
	all_hints "(\from+1)", \to
	.endif
	.endm

	all_hints from=0, to=63
	all_hints from=64, to=127

	//
	// SYSL
	//

	sysl	x7, #3, C15, C7, #7

	//
	// BARRIERS
	//

	.macro	all_barriers op, from=0, to=15
	\op	\from
	.if	\to-\from
	all_barriers \op, "(\from+1)", \to
	.endif
	.endm

	all_barriers	op=dsb, from=0, to=15
	all_barriers	op=dmb, from=0, to=15
	all_barriers	op=isb, from=0, to=15

	isb

	//
	// PREFETCHS
	//

	.macro	all_prefetchs op, from=0, to=31
	\op	\from, LABEL1
	\op	\from, [sp, x15, lsl #0]
	\op	\from, [x7, w30, uxtw #3]
	\op	\from, [x3, #24]
	.if	\to-\from
	all_prefetchs \op, "(\from+1)", \to
	.endif
	.endm

	all_prefetchs	op=prfm, from=0, to=31

	//
	// PREFETCHS with named operation
	//

	.irp op, pld, pli, pst
	.irp l, l1, l2, l3
	.irp t, keep, strm
	prfm	\op\l\t, [x3, #24]
	.endr
	.endr
	.endr
