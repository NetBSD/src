#	$NetBSD: Makefile,v 1.6 1997/10/24 09:00:52 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	nfsstat
CPPFLAGS+=-DNFS
DPADD=  ${LIBKVM}
LDADD= -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
