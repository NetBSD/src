#	$NetBSD: Makefile,v 1.7 2000/06/13 12:39:43 simonb Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	nfsstat
CPPFLAGS+=-DNFS
DPADD=  ${LIBKVM}
LDADD= -lkvm

.include <bsd.prog.mk>
