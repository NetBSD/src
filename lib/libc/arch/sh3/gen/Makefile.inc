#	$NetBSD: Makefile.inc,v 1.11 2003/08/01 17:03:54 lukem Exp $

SRCS+=	_setjmp.S setjmp.S sigsetjmp.S __setjmp14.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.c swapcontext.S

SRCS+=	_lwp.c

SRCS+=	fabs.c flt_rounds.c bswap16.c bswap32.c bswap64.c

# Common ieee754 constants and functions
SRCS+=	infinity_ieee754.c nanf_ieee754.c
SRCS+=	frexp_ieee754.c isinf_ieee754.c isinfl_ieee754.c isnan_ieee754.c
SRCS+=	isnanl_ieee754.c ldexp_ieee754.c modf_ieee754.c

SRCS+=	ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstr_i4.S \
	movstrSI.S mulsi3.S sdivsi3.S udivsi3.S

KMSRCS+= ashiftrt.S ashlsi3.S ashrsi3.S lshrsi3.S movstr.S movstr_i4.S \
	movstrSI.S mulsi3.S sdivsi3.S udivsi3.S

SRCS.sh3.gen=	Lint_swapcontext.c
LSRCS+=		${SRCS.sh3.gen}
DPSRCS+=	${SRCS.sh3.gen}
CLEANFILES+=	${SRCS.sh3.gen}
