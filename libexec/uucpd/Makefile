#	$NetBSD: Makefile,v 1.6.2.1 1997/11/08 07:26:04 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

PROG=	uucpd
NOMAN=	noman
CPPFLAGS+= -DBSDINETD
LDADD+= -lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
