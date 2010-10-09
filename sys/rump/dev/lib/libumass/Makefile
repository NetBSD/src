#	$NetBSD: Makefile,v 1.5.2.3 2010/10/09 03:32:42 yamt Exp $
#

.PATH:	${.CURDIR}/../../../../dev/usb

LIB=	rumpdev_umass
IOCONF=	UMASS.ioconf

.if ${MACHINE} == "i386"
SRCS=	umass.c umass_isdata.c umass_quirks.c umass_scsipi.c

SRCS+=	component.c
.else
SRCS=	dummy.c
.endif

CPPFLAGS+=	-I${.CURDIR}/../libscsipi/opt# -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
