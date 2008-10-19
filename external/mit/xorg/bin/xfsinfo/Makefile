#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:01 haad Exp $

.include <bsd.own.mk>

PROG=	xfsinfo

LDADD+=	-lFS
DPADD+=	${LIBFS}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
