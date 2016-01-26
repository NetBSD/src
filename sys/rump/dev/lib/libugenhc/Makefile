#	$NetBSD: Makefile,v 1.13 2016/01/26 23:12:16 pooka Exp $
#

LIB=	rumpdev_ugenhc
COMMENT=USB host controller using /dev/ugen

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
IOCONF= UGENHC.ioconf
RUMP_COMPONENT=ioconf

SRCS=	ugenhc.c ugenhc_dma.c

RUMPCOMP_USER_SRCS=	ugenhc_user.c
.else
.PATH:	${.CURDIR}/../libusb
SRCS=	dummy.c
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
