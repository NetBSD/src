#	$NetBSD: Makefile,v 1.2.6.2 2010/04/30 14:44:24 uebayasi Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
