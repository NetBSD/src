#	$NetBSD: Makefile,v 1.6 2015/10/19 16:16:35 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons
COMMENT=Workstation console support

IOCONF=	WSCONS.ioconf

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	wscons_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
