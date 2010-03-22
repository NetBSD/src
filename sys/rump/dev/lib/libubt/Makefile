#	$NetBSD: Makefile,v 1.1 2010/03/22 12:14:51 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_ubt
IOCONF= UBT.ioconf

SRCS=	ubt.c

SRCS+=	ubt_at_usb.c

CPPFLAGS+= -I${RUMPTOP}/net/lib/libnetbt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
