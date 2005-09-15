#	$NetBSD: Makefile.inc,v 1.1 2005/09/15 22:44:58 uwe Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	compat_setjmp.S compat__setjmp.S compat_sigsetjmp.S

SRCS.compat.sh3.gen=Lint__setjmp.c

LSRCS+=		${SRCS.compat.sh3.gen}
DPSRCS+=	${SRCS.compat.sh3.gen}
CLEANFILES+=	${SRCS.compat.sh3.gen}
