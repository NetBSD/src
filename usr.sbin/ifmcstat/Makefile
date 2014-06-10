# $NetBSD: Makefile,v 1.4 2014/06/10 09:38:30 joerg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ifmcstat
MAN=	ifmcstat.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}

CPPFLAGS+=	-DINET6

.include <bsd.prog.mk>
