#	$NetBSD: Makefile.inc,v 1.13 2003/08/01 17:03:53 lukem Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	fabs.c flt_rounds.c
SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c
SRCS+=	fpgetsticky.c fpsetsticky.c
SRCS+=	syncicache.c
SRCS+= _lwp.c makecontext.c resumecontext.c swapcontext.S

# Common ieee754 constants and functions
SRCS+=	infinity_ieee754.c nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isinfl_ieee754.c isnan_ieee754.c
SRCS+=	isnanl_ieee754.c ldexp_ieee754.c modf_ieee754.c

SRCS.powerpc.gen=Lint_swapcontext.c
LSRCS+=		${SRCS.powerpc.gen}
DPSRCS+=	${SRCS.powerpc.gen}
CLEANFILES+=	${SRCS.powerpc.gen}
