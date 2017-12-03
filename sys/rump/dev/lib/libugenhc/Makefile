#	$NetBSD: Makefile,v 1.2.24.3 2017/12/03 11:39:09 jdolecek Exp $
#

LIB=	rumpdev_ugenhc
COMMENT=USB host controller using /dev/ugen

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
IOCONF= UGENHC.ioconf
RUMP_COMPONENT=ioconf

SRCS=	ugenhc.c

RUMPCOMP_USER_SRCS=	ugenhc_user.c
.else
.PATH:	${.CURDIR}/../libusb
SRCS=	dummy.c
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
