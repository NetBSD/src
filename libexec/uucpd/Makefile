#	$NetBSD: Makefile,v 1.4 1997/10/07 11:03:00 mrg Exp $
#	from: @(#)Makefile	5.3 (Berkeley) 6/25/90

WARNS?=	1
CFLAGS+= -DBSDINETD
PROG=	uucpd
NOMAN=	noman
LDADD+= -lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
