#	$NetBSD: Makefile.inc,v 1.1 2005/09/13 01:44:08 christos Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	compat_setjmp.S compat__setjmp.S compat_sigsetjmp.S

SRCS.i386.gen=Lint__setjmp.c

LSRCS+=		${SRCS.i386.gen}
DPSRCS+=	${SRCS.i386.gen}
CLEANFILES+=	${SRCS.i386.gen}
