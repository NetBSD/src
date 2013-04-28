#	$NetBSD: Makefile,v 1.6 2013/04/28 10:25:41 pooka Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER= #ioctl

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
