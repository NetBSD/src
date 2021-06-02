#	$NetBSD: Makefile,v 1.4.24.1 2021/06/02 14:46:22 cjep Exp $

.include <bsd.own.mk>

PROG=	xmessage
SRCS=	xmessage.c makeform.c readfile.c

APPDEFS=Xmessage Xmessage-color

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man
.PATH:	${X11SRCDIR.${PROG}}/app-defaults

.include <bsd.x11.mk>
.include <bsd.prog.mk>
