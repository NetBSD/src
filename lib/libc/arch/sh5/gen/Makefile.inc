#	$NetBSD: Makefile.inc,v 1.13 2004/01/15 19:43:45 kleink Exp $

SRCS+=	__setjmp14.S __sigsetjmp14.S _setjmp.S \
	alloca.S \
	byte_swap_2.S byte_swap_4.S byte_swap_8.S \
	fabs.S flt_rounds.S \

SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c \
	frexp_ieee754.c \
	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c \
	isfinitef_ieee754.c isfinited_ieee754.c \
	isinf_ieee754.c isinfl_dbl_ieee754.c \
	isnan_ieee754.c isnanl_dbl_ieee754.c \
	signbitf_ieee754.c signbitd_ieee754.c \
	ldexp_ieee754.c \
	modf_ieee754.c \
	nanf_ieee754.c

SRCS+=	fpgetmask.S fpgetround.S fpgetsticky.S \
	fpsetmask.S fpsetround.S fpsetsticky.S

SRCS+=	makecontext.c resumecontext.c swapcontext.S _lwp.c

SRCS.sh5.gen=	Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c Lint_swapcontext.c
LSRCS+=		${SRCS.sh5.gen}
DPSRCS+=	${SRCS.sh5.gen}
CLEANFILES+=	${SRCS.sh5.gen}
