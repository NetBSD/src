#	$NetBSD: Makefile.inc,v 1.2 2003/08/01 17:03:57 lukem Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S

SRCS.x86_64.stdlib=Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
LSRCS+=		${SRCS.x86_64.stdlib}
DPSRCS+=	${SRCS.x86_64.stdlib}
CLEANFILES+=	${SRCS.x86_64.stdlib}
