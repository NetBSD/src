#	$NetBSD: Makefile.inc,v 1.8 1998/10/05 02:34:18 thorpej Exp $

SRCS+=	alloca.S fabs.S frexp.c infinity.c isinf.c modf.S
SRCS+=	flt_rounds.S fpgetmask.S fpgetround.S fpgetsticky.S fpsetmask.S \
	fpsetround.S fpsetsticky.S
SRCS+=	adddf3.S addsf3.S ashlsi3.S ashrsi3.S cmpdf2.S cmpsf2.S divdf3.S \
	divsf3.S divsi3.S extendsfdf2.S fixdfsi.S fixunsdfsi.S \
	floatsidf.S lshlsi3.S lshrsi3.S modsi3.S muldf3.S mulsf3.S mulsi3.S \
	negdf2.S negsf2.S subdf3.S subsf3.S truncdfsf2.S udivsi3.S \
	umodsi3.S umulsi3.S

SRCS+=	setjmp.S __setjmp14.S
SRCS+=	_setjmp.S
SRCS+=	sigsetjmp.S __sigsetjmp14.S

.ifdef(M68040)
SRCS+=	ldexp.c
.elifdef(M68060)
SRCS+=	ldexp.c
.else
SRCS+=	ldexp.S 
.endif
