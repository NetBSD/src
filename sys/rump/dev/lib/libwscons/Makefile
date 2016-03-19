#	$NetBSD: Makefile,v 1.4.6.3 2016/03/19 11:30:36 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons
COMMENT=Workstation console support

IOCONF=	WSCONS.ioconf

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	wscons_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
