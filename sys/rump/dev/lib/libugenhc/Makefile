#	$NetBSD: Makefile,v 1.2.2.2 2010/03/11 15:04:33 yamt Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
