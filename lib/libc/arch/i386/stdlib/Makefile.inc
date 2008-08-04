#	$NetBSD: Makefile.inc,v 1.6.6.2 2008/08/04 21:29:29 matt Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S llabs.S

# objects built from C sources
SRCS+=	erand48_ieee754.c

SRCS.i386.stdlib=Lint_abs.c Lint_labs.c Lint_llabs.c Lint_imaxabs.c
SRCS.i386.stdlib+=Lint_div.c Lint_ldiv.c
LSRCS+=		${SRCS.i386.stdlib}
DPSRCS+=	${SRCS.i386.stdlib}
CLEANFILES+=	${SRCS.i386.stdlib}
