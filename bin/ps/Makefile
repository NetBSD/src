#	$NetBSD: Makefile,v 1.16 1995/10/26 00:48:59 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=	ps
SRCS=	fmt.c keyword.c nlist.c print.c ps.c
DPADD=	${LIBM} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
