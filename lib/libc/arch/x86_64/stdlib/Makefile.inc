#	$NetBSD: Makefile.inc,v 1.3 2006/03/22 22:03:58 drochner Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S

SRCS+=	erand48_ieee754.c

SRCS.x86_64.stdlib=Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
LSRCS+=		${SRCS.x86_64.stdlib}
DPSRCS+=	${SRCS.x86_64.stdlib}
CLEANFILES+=	${SRCS.x86_64.stdlib}
