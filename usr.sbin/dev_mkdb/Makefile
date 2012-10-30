#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.7.6.1 2012/10/30 19:00:32 yamt Exp $

PROG=	dev_mkdb
MAN=	dev_mkdb.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>

LDFLAGS+=	-Wl,--no-fatal-warnings
