# $NetBSD: Makefile,v 1.3.60.1 2014/08/20 00:05:08 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifmcstat
MAN=	ifmcstat.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

CPPFLAGS+=	-DINET6

.include <bsd.prog.mk>
