
#define	CCFSZ	96

	.file	"str0.s"

	.text	
	.globl	start

start:
	/*
	 * Set up a stack.
	 */
	set	start, %o1
	save	%o1, -CCFSZ, %sp

	/*
	 * Relocate.
	 */
1:	call	2f
	 nop
2:	add	%o7, (start-1b), %l0
	set	start, %l1
	set	_end, %o0
	sub	%o0, %l1, %l2		! length
3:	ld	[%l0], %o0
	add	%l0, 4, %l0
	st	%o0, [%l1]
	subcc	%l2, 4, %l2
	bg	3b
	 add	%l1, 4, %l1

	set	4f, %g1
	jmp	%g1
	 nop

4:
	/*
	 * Clear BSS
	 */
	set     _edata, %o0             ! bzero(edata, end - edata)
	set     _end, %o1
	call    _bzero
	 sub    %o1, %o0, %o1

	/*
	 * Save address of PROM vector (passed in %i0).
	 */
	sethi	%hi(_promvec), %o0
	st	%i0, [%o0 + %lo(_promvec)]

	/*
	 * That's it, really...
	 */
	call	_main
	 mov	%i0, %o0

	ret
	 restore

/*
 * XXX - Space saving .div & .rem routines (small & non-negative numbres only)
 */
	.align	4
	.global .div, .udiv
! int n = 0; while (a >= b) { a -= b; n++; }; return n;
.div:
.udiv:
	cmp	%o0, %o1
	bl	2f
	 mov	0, %o5
1:
	sub	%o0, %o1, %o0
	cmp	%o0, %o1
	bge	1b
	 add	%o5, 1, %o5
2:
	retl
	 mov	%o5, %o0

	.align	4
	.global .rem, .urem
! while (a>=b) a -= b; return a;
.rem:
.urem:
	cmp	%o0, %o1
	bl	2f
	 nop
	sub	%o0, %o1, %o0
1:
	cmp	%o0, %o1
	bge,a	1b
	 sub	%o0, %o1, %o0
2:
	retl
	 nop

