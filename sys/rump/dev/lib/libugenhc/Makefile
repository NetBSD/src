#	$NetBSD: Makefile,v 1.6.4.1 2014/05/18 17:46:16 rmind Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c ugenhc_dma.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	ugenhc_user.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
