#	$NetBSD: Makefile,v 1.7 2013/09/19 17:55:22 pooka Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c ugenhc_dma.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER= #ioctl

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
