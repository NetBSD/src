#	$NetBSD: Makefile,v 1.10 2015/10/19 16:16:34 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_umass
COMMENT=USB mass storage driver

IOCONF=	UMASS.ioconf

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SRCS=	umass.c umass_isdata.c umass_quirks.c umass_scsipi.c

SRCS+=	umass_component.c
.else
SRCS=	dummy.c
.endif

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
