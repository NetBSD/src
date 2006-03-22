#	$NetBSD: Makefile.inc,v 1.5 2006/03/22 22:03:58 drochner Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S llabs.S

# objects built from C sources
SRCS+=	erand48_ieee754.c

SRCS.i386.stdlib=Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
LSRCS+=		${SRCS.i386.stdlib}
DPSRCS+=	${SRCS.i386.stdlib}
CLEANFILES+=	${SRCS.i386.stdlib}
