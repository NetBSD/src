# $NetBSD: Makefile.inc,v 1.1.2.3 2002/03/08 21:34:45 nathanw Exp $

SRCS+=	alloca.S byte_swap_2.S byte_swap_4.S bswap64.c divsi3.S \
	fabs.c flt_rounds.c \
	infinity.c

# Common ieee754 constants and functions
SRCS+=	ieee754_nanf.c	# infinity is ``different'' on arm, use local version
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c ieee754_ldexp.c
SRCS+=	ieee754_modf.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c swapcontext.S

SRCS+=	_lwp.c

LSRCS+= Lint_bswap16.c Lint_bswap32.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c

LSRCS+=	Lint_swapcontext.c
DPSRCS+= Lint_swapcontext.c
