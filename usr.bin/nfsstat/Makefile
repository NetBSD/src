#	$NetBSD: Makefile,v 1.6.8.1 2000/06/23 16:39:50 minoura Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	nfsstat
CPPFLAGS+=-DNFS
DPADD=  ${LIBKVM}
LDADD= -lkvm

.include <bsd.prog.mk>
