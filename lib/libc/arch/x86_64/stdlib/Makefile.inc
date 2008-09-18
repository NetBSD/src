#	$NetBSD: Makefile.inc,v 1.3.20.1 2008/09/18 04:39:20 wrstuden Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	abs.S div.S labs.S ldiv.S

SRCS+=	erand48_ieee754.c

SRCS.x86_64.stdlib=Lint_abs.c Lint_labs.c Lint_llabs.c Lint_imaxabs.c
SRCS.x86_64.stdlib+=Lint_div.c Lint_ldiv.c 
LSRCS+=		${SRCS.x86_64.stdlib}
DPSRCS+=	${SRCS.x86_64.stdlib}
CLEANFILES+=	${SRCS.x86_64.stdlib}
