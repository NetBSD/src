#	$NetBSD: Makefile,v 1.14 1999/09/12 01:19:25 chs Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	fstat
SRCS=	fstat.c isofs.c ntfs.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
