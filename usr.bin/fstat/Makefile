#	$NetBSD: Makefile,v 1.15 2000/01/17 16:14:39 itojun Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	fstat
SRCS=	fstat.c isofs.c ntfs.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

CPPFLAGS+=-DINET6

.include <bsd.prog.mk>
