#	$NetBSD: Makefile.inc,v 1.7 1998/09/29 20:24:36 thorpej Exp $

# objects built from assembler sources (need lint stubs)
ASSRCS+=alloca.S fabs.S frexp.S ldexp.S modf.S

ASSRCS+=setjmp.S __setjmp14.S
ASSRCS+=_setjmp.S
ASSRCS+=sigsetjmp.S __sigsetjmp14.S

# objects built from C sources
SRCS+=	flt_rounds.c fpgetmask.c fpgetround.c fpgetsticky.c fpsetmask.c \
	fpsetround.c fpsetsticky.c infinity.c isinf.c
