#	$NetBSD: Makefile.inc,v 1.12 2003/08/01 17:03:55 lukem Exp $

SRCS+=	fabs.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

# Common ieee754 constants and functions
SRCS+=	infinity_ieee754.c nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isinfl_ieee754.c isnan_ieee754.c
SRCS+=	isnanl_ieee754.c ldexp_ieee754.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c _lwp.c swapcontext.S

SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S
SRCS+=	bswap16.c bswap32.c bswap64.c

SRCS.sparc.gen=	Lint_swapcontext.c
LSRCS+=		${SRCS.sparc.gen}
DPSRCS+=	${SRCS.sparc.gen}
CLEANFILES+=	${SRCS.sparc.gen}
