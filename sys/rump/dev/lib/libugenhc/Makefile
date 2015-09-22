#	$NetBSD: Makefile,v 1.8.6.2 2015/09/22 12:06:14 skrll Exp $
#

LIB=	rumpdev_ugenhc

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
IOCONF= UGENHC.ioconf

SRCS=	ugenhc.c ugenhc_at_mainbus.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	ugenhc_user.c
.else
.PATH:	${.CURDIR}/../libusb
SRCS=	dummy.c
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
