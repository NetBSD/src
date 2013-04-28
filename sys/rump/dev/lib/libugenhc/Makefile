#	$NetBSD: Makefile,v 1.3 2013/04/28 09:58:11 pooka Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPUSER_COMP= ioctl

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
