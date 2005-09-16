#	$NetBSD: Makefile.inc,v 1.20 2005/09/16 18:27:34 drochner Exp $

SRCS+=	fabs.S 

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c
SRCS+=	frexp_ieee754.c
SRCS+=	isinff_ieee754.c isinfd_ieee754.c
SRCS+=	isnanf_ieee754.c isnand_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c
SRCS+=	ldexp_ieee754.c modf_ieee754.c
SRCS+=	signbitf_ieee754.c signbitd_ieee754.c

SRCS+=	nanf.c

SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
SRCS+=	__setjmp14.S
SRCS+=	__sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c
SRCS+=	_resumecontext.S

SRCS.alpha.gen=	Lint_bswap16.c Lint_bswap32.c Lint_swapcontext.c
LSRCS+=		${SRCS.alpha.gen}
DPSRCS+=	${SRCS.alpha.gen}
CLEANFILES+=	${SRCS.alpha.gen}

SRCS+=	makecontext.c resumecontext.c swapcontext.S _lwp.c

