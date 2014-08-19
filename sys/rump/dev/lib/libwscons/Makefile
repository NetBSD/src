#	$NetBSD: Makefile,v 1.3.24.1 2014/08/20 00:04:39 tls Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons
IOCONF=	WSCONS.ioconf

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	wscons_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
