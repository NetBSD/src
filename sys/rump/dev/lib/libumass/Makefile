#	$NetBSD: Makefile,v 1.6.22.1 2014/05/18 17:46:16 rmind Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_umass
IOCONF=	UMASS.ioconf

.if ${MACHINE} == "i386"
SRCS=	umass.c umass_isdata.c umass_quirks.c umass_scsipi.c

SRCS+=	umass_component.c
.else
SRCS=	dummy.c
.endif

CPPFLAGS+=	-I${.CURDIR}/../libscsipi/opt# -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
