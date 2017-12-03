#	$NetBSD: Makefile,v 1.6.18.2 2017/12/03 11:39:10 jdolecek Exp $
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
