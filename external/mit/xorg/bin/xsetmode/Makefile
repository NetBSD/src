#	$NetBSD: Makefile,v 1.1.1.1.72.1 2021/06/02 14:46:24 cjep Exp $

.include <bsd.own.mk>

PROG=	xsetmode

LDADD+=	${XLIBLDADD_XI}
DPADD+=	${XLIBDPADD_XI}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
