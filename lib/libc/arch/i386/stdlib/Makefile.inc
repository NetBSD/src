#	$NetBSD: Makefile.inc,v 1.3 2000/03/06 18:32:24 kleink Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S llabs.S

LSRCS+= Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
DPSRCS+= Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
