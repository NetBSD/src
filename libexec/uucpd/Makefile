#	$NetBSD: Makefile,v 1.7 1997/10/22 06:17:43 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	uucpd
NOMAN=	noman
CPPFLAGS+= -DBSDINETD
LDADD+= -lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
