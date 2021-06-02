#	$NetBSD: Makefile,v 1.3.8.1 2021/06/02 15:04:23 cjep Exp $

.include <bsd.own.mk>

PROG=	xbiff
SRCS=	xbiff.c Mailbox.c

LDADD+=	${XLIBLDADD_XAW}
DPADD+=	${XLIBDPADD_XAW}

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include
CPPFLAGS+=	-D_CONST_X_STRING

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
