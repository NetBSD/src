#	from: @(#)Makefile	5.3 (Berkeley) 6/25/90
#	$Id: Makefile,v 1.3 1993/10/07 02:17:52 cgd Exp $

CFLAGS+= -DBSDINETD
PROG=	uucpd
NOMAN=	noman
LDADD+= -lcrypt
DPADD+=	${LIBCRYPT}

.include <bsd.prog.mk>
