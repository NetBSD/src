#	$NetBSD: Makefile,v 1.8 2015/06/13 23:33:20 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_umass
IOCONF=	UMASS.ioconf

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SRCS=	umass.c umass_isdata.c umass_quirks.c umass_scsipi.c

SRCS+=	umass_component.c
.else
SRCS=	dummy.c
.endif

CPPFLAGS+=	-I${.CURDIR}/../libscsipi/opt# -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
