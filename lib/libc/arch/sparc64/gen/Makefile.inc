#	$NetBSD: Makefile.inc,v 1.11 2003/10/25 22:31:20 kleink Exp $

SRCS+=	fabs.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c
SRCS+=	nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isnan_ieee754.c
SRCS+=	ldexp_ieee754.c

SRCS+=	infinityl.c isinfl.c isnanl.c

SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c _lwp.c swapcontext.S

SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S

SRCS.sparc64.gen=Lint_swapcontext.c
LSRCS+=		${SRCS.sparc64.gen}
DPSRCS+=	${SRCS.sparc64.gen}
CLEANFILES+=	${SRCS.sparc64.gen}
