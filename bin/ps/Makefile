#	$NetBSD: Makefile,v 1.17 1997/07/20 20:37:53 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

WARNS=	1
PROG=	ps
SRCS=	fmt.c keyword.c nlist.c print.c ps.c
DPADD=	${LIBM} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
