#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:30 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xfsinfo

LDADD+=	-lFS
DPADD+=	${LIBFS}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
