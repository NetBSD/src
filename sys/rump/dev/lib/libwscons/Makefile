#	$NetBSD: Makefile,v 1.7 2016/01/26 23:12:17 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons
COMMENT=Workstation console support

IOCONF=	WSCONS.ioconf

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	wscons_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
