#	$NetBSD: Makefile,v 1.22 2009/04/14 22:15:20 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

USE_FORT?= yes	# setuid
PROG=	fstat
SRCS=	fstat.c isofs.c ntfs.c ptyfs.c tmpfs.c misc.c
DPADD=	${LIBKVM}
LDADD=	-lkvm
BINGRP=	kmem
BINMODE=2555

CPPFLAGS+= -I${NETBSDSRCDIR}/sys

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>
