#	$NetBSD: Makefile,v 1.19 2006/02/10 16:01:45 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

WARNS=	3
PROG=	fstat
SRCS=	fstat.c isofs.c ntfs.c ptyfs.c tmpfs.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>
