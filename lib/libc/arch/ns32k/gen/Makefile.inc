#	$NetBSD: Makefile.inc,v 1.6 1997/12/07 00:19:16 matthias Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=_setjmp.S alloca.S fabs.S frexp.S ldexp.S modf.S \
	setjmp.S sigsetjmp.S

# objects built from C sources
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c infinity.c isinf.c
