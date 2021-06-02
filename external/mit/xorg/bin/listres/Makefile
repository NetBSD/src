#	$NetBSD: Makefile,v 1.2.36.1 2021/06/02 14:46:19 cjep Exp $

.include <bsd.own.mk>

PROG=	listres

LDADD+= ${XLIBLDADD_XAW}
DPADD+= ${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
