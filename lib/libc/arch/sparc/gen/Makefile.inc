#	$NetBSD: Makefile.inc,v 1.15 2004/01/15 19:43:45 kleink Exp $

SRCS+=	fabs.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c
SRCS+=	nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isinfl_dbl_ieee754.c isnan_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c
SRCS+=	isnanl_dbl_ieee754.c ldexp_ieee754.c

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
