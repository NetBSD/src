#	$NetBSD: Makefile.inc,v 1.4 2003/08/01 17:03:48 lukem Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S llabs.S

SRCS.i386.stdlib=Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
LSRCS+=		${SRCS.i386.stdlib}
DPSRCS+=	${SRCS.i386.stdlib}
CLEANFILES+=	${SRCS.i386.stdlib}
