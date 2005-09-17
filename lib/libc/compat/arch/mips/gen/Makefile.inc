#	$NetBSD: Makefile.inc,v 1.1 2005/09/17 11:49:39 tsutsui Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	compat_setjmp.S compat__setjmp.S compat_sigsetjmp.S

SRCS.mips.gen=Lint__setjmp.c

LSRCS+=		${SRCS.mips.gen}
DPSRCS+=	${SRCS.mips.gen}
CLEANFILES+=	${SRCS.mips.gen}
