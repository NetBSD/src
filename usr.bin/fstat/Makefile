#	$NetBSD: Makefile,v 1.16 2004/12/12 22:41:03 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	fstat
SRCS=	fstat.c isofs.c ntfs.c ptyfs.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

CPPFLAGS+= -I${NETBSDSRCDIR}/sys
CPPFLAGS+=-DINET6

.include <bsd.prog.mk>
