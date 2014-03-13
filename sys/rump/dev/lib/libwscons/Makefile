#	$NetBSD: Makefile,v 1.4 2014/03/13 01:48:08 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons
IOCONF=	WSCONS.ioconf

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	wscons_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
