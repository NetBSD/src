#	$NetBSD: Makefile.inc,v 1.12 1999/12/23 10:15:07 kleink Exp $

SRCS+=	alloca.S fabs.S frexp.c infinity.c isinf.c isnan.c modf.S nanf.c
SRCS+=	flt_rounds.S fpgetmask.S fpgetround.S fpgetsticky.S fpsetmask.S \
	fpsetround.S fpsetsticky.S
SRCS+=	adddf3.S addsf3.S ashlsi3.S ashrsi3.S cmpdf2.S cmpsf2.S divdf3.S \
	divsf3.S divsi3.S extendsfdf2.S fixdfsi.S fixunsdfsi.S \
	floatsidf.S lshlsi3.S lshrsi3.S modsi3.S muldf3.S mulsf3.S mulsi3.S \
	negdf2.S negsf2.S subdf3.S subsf3.S truncdfsf2.S udivsi3.S \
	umodsi3.S umulsi3.S
SRCS+=	bswap16.S bswap32.S bswap64.S

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

.ifdef M68040
SRCS+=	ldexp_040.c
.elifdef M68060
SRCS+=	ldexp_040.c
.else
SRCS+=	ldexp_881.c
.endif
LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
