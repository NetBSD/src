#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:40:58 haad Exp $

.include <bsd.own.mk>

PROG=	showfont

LDADD+=	-lFS
DPADD+=	${LIBFS}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
