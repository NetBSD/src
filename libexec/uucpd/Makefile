#	$NetBSD: Makefile,v 1.5 1997/10/08 09:07:40 mrg Exp $
#	from: @(#)Makefile	5.3 (Berkeley) 6/25/90

CFLAGS+= -DBSDINETD
PROG=	uucpd
NOMAN=	noman
LDADD+= -lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
