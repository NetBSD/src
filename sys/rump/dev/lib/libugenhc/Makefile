#	$NetBSD: Makefile,v 1.8 2014/03/13 01:38:11 pooka Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c ugenhc_dma.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	ugenhc_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
