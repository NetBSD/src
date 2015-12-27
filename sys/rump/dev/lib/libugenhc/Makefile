#	$NetBSD: Makefile,v 1.8.6.3 2015/12/27 12:10:11 skrll Exp $
#

LIB=	rumpdev_ugenhc
COMMENT=USB host controller using /dev/ugen

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
IOCONF= UGENHC.ioconf
RUMP_COMPONENT=ioconf

SRCS=	ugenhc.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpkern

RUMPCOMP_USER_SRCS=	ugenhc_user.c
.else
.PATH:	${.CURDIR}/../libusb
SRCS=	dummy.c
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
