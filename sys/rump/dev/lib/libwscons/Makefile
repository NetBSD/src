#	$NetBSD: Makefile,v 1.3.14.1 2014/05/22 11:41:13 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons
IOCONF=	WSCONS.ioconf

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	wscons_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
