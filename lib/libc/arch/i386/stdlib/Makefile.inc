#	$NetBSD: Makefile.inc,v 1.2.2.2 1997/11/08 21:56:37 veego Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S

LSRCS+= Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c
DPSRCS+= Lint_abs.c Lint_div.c Lint_labs.c Lint_ldiv.c
