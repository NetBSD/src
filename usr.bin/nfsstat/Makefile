#	$NetBSD: Makefile,v 1.5.2.1 1997/11/08 10:37:50 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	nfsstat
CPPFLAGS+=-DNFS
DPADD=  ${LIBKVM}
LDADD= -lkvm
BINGRP=	kmem
BINMODE=2555

.include <bsd.prog.mk>
