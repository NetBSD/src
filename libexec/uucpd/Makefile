#	$NetBSD: Makefile,v 1.6 1997/10/08 09:12:46 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/4/93

CFLAGS+= -DBSDINETD
PROG=	uucpd
NOMAN=	noman
LDADD+= -lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
