#	$NetBSD: Makefile,v 1.2.24.1 2013/06/23 06:20:27 tls Exp $
#

LIB=	rumpdev_ugenhc
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER= #ioctl

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
