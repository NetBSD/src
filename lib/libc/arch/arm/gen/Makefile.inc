# $NetBSD: Makefile.inc,v 1.9 2003/10/26 16:00:17 kleink Exp $

SRCS+=	alloca.S byte_swap_2.S byte_swap_4.S bswap64.c divsi3.S \
	fabs.c flt_rounds.c \
	infinity.c infinityl.c

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c nanf_ieee754.c
SRCS+=	isinfl_dbl_ieee754.c isnanl_dbl_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isnan_ieee754.c
SRCS+=	ldexp_ieee754.c modf_ieee754.c

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c swapcontext.S

SRCS+=	_lwp.c

SRCS.arm.gen=	Lint_bswap16.c Lint_bswap32.c Lint_swapcontext.c
LSRCS+=		${SRCS.arm.gen}
DPSRCS+=	${SRCS.arm.gen}
CLEANFILES+=	${SRCS.arm.gen}
