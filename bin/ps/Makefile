#	$NetBSD: Makefile,v 1.14 1995/05/18 20:33:20 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/2/93

PROG=	ps
SRCS=	keyword.c nlist.c print.c ps.c
DPADD=	${LIBMATH} ${LIBKVM}
LDADD=	-lm -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
