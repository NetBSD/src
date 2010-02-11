#	$NetBSD: Makefile,v 1.1 2010/02/11 02:22:10 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wscons

SRCS=   wsevent.c wskbd.c wskbdutil.c wsmouse.c

SRCS+=	component.c

CPPFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs -I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
