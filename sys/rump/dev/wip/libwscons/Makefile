#	$NetBSD: Makefile,v 1.1 2010/01/11 02:12:51 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	component.c

CPPFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
