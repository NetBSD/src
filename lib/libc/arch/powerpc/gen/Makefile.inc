#	$NetBSD: Makefile.inc,v 1.18 2004/01/15 19:43:45 kleink Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S
SRCS+=	bswap16.c bswap32.c bswap64.c
SRCS+=	fabs.c flt_rounds.c
SRCS+=	syncicache.c
SRCS+= _lwp.c makecontext.c resumecontext.c swapcontext.S

# Common ieee754 constants and functions
SRCS+=	infinityf_ieee754.c infinity_ieee754.c infinityl_dbl_ieee754.c
SRCS+=	nanf_ieee754.c
SRCS+=	fpclassifyf_ieee754.c fpclassifyd_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isinfl_dbl_ieee754.c isnan_ieee754.c
SRCS+=	isfinitef_ieee754.c isfinited_ieee754.c
SRCS+=	isnanl_dbl_ieee754.c ldexp_ieee754.c modf_ieee754.c
SRCS+=	signbitf_ieee754.c signbitd_ieee754.c

.if (${MKSOFTFLOAT} == "no")
SRCS+=	fpgetround.c fpsetround.c fpgetmask.c fpsetmask.c
SRCS+=	fpgetsticky.c fpsetsticky.c
.endif

SRCS.powerpc.gen=Lint_swapcontext.c
LSRCS+=		${SRCS.powerpc.gen}
DPSRCS+=	${SRCS.powerpc.gen}
CLEANFILES+=	${SRCS.powerpc.gen}
