#	$NetBSD: Makefile.inc,v 1.17 2005/10/16 17:15:38 christos Exp $

SRCS+=	fabs.S modf.S
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c
SRCS+=	frexp_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c
SRCS+=	isinff_ieee754.c isinfd_ieee754.c
SRCS+=	isnanf_ieee754.c isnand_ieee754.c
SRCS+=	ldexp_ieee754.c
SRCS+=	signbitf_ieee754.c signbitd_ieee754.c

SRCS+=	nanf.c infinityl.c
SRCS+=	fpclassifyl.c isfinitel.c isinfl.c isnanl.c signbitl.c

SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	setjmp.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S longjmp.c

SRCS+=	makecontext.c resumecontext.c _lwp.c swapcontext.S

SRCS+=	fixunsdfsi.S mul.S umul.S saveregs.S

SRCS.sparc64.gen=Lint_swapcontext.c
LSRCS+=		${SRCS.sparc64.gen}
DPSRCS+=	${SRCS.sparc64.gen}
CLEANFILES+=	${SRCS.sparc64.gen}
