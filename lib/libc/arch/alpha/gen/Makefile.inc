#	$NetBSD: Makefile.inc,v 1.13 2003/08/01 17:03:46 lukem Exp $

SRCS+=	fabs.S 

# Common ieee754 constants and functions
SRCS+=	infinity_ieee754.c nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isinfl_ieee754.c isnan_ieee754.c
SRCS+=	isnanl_ieee754.c ldexp_ieee754.c modf_ieee754.c

SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c
SRCS+=	_setjmp.S
SRCS+=	setjmp.S __setjmp14.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S
SRCS+=	byte_swap_2.S byte_swap_4.S bswap64.c

SRCS.alpha.gen=	Lint_bswap16.c Lint_bswap32.c Lint_swapcontext.c
LSRCS+=		${SRCS.alpha.gen}
DPSRCS+=	${SRCS.alpha.gen}
CLEANFILES+=	${SRCS.alpha.gen}

SRCS+=	makecontext.c resumecontext.c swapcontext.S _lwp.c

