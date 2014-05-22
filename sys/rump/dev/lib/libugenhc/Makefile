#	$NetBSD: Makefile,v 1.2.14.1 2014/05/22 11:41:13 yamt Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c ugenhc_dma.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	ugenhc_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
