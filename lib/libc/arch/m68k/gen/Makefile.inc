#	$NetBSD: Makefile.inc,v 1.12.6.5 2002/03/22 20:41:52 nathanw Exp $

SRCS+=	alloca.S fabs.S

# Common ieee754 constants and functions
SRCS+=	ieee754_infinity.c ieee754_nanf.c
SRCS+=	ieee754_frexp.c ieee754_isinf.c ieee754_isnan.c

SRCS+=	ashlsi3.S ashrsi3.S \
	lshlsi3.S lshrsi3.S \
	negdf2.S negsf2.S
SRCS+=	bswap16.S bswap32.S bswap64.S
SRCS+=	_lwp.c

# 68000-based machines build with a libgcc that includes
# much of the (soft)float and integer support that would 
# otherwise be compiled here.
.if	${MACHINE_ARCH} == "m68000"
SRCS+=	ieee754_modf.c			# generic ieee754 version
SRCS+=	flt_rounds_softfloat.S
.else
SRCS+=	modf.S
SRCS+=	flt_rounds.S fpgetmask.S fpgetround.S fpgetsticky.S fpsetmask.S \
	fpsetround.S fpsetsticky.S
SRCS+=	adddf3.S addsf3.S cmpdf2.S cmpsf2.S divdf3.S \
	divsf3.S divsi3.S extendsfdf2.S fixdfsi.S fixunsdfsi.S \
	floatsidf.S modsi3.S muldf3.S mulsf3.S mulsi3.S \
	subdf3.S subsf3.S truncdfsf2.S udivsi3.S \
	umodsi3.S umulsi3.S
.endif

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

SRCS+=	makecontext.c resumecontext.S swapcontext.S

.ifdef M68040
SRCS+=	ieee754_ldexp.c			# generic ieee754 version
.elifdef M68060
SRCS+=	ieee754_ldexp.c			# generic ieee754 version
.elif	${MACHINE_ARCH} == "m68000"
SRCS+=	ieee754_ldexp.c			# generic ieee754 version
.else
SRCS+=	ldexp_881.c
.endif
LSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c
DPSRCS+= Lint_bswap16.c Lint_bswap32.c Lint_bswap64.c

LSRCS+=	Lint_resumecontext.c Lint_swapcontext.c
DPSRCS+= Lint_resumecontext.c Lint_swapcontext.c
