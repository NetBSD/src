#	$NetBSD: Makefile.inc,v 1.1 2001/06/19 00:25:04 fvdl Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S

LSRCS+= Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
DPSRCS+= Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c Lint_llabs.c
