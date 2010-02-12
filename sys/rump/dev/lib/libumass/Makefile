#	$NetBSD: Makefile,v 1.2 2010/02/12 12:07:12 pooka Exp $
#
#
# umass.  includes scsibus+sd for simplicity due to the umass code
# needing compile time knowledge of its presence.  fixxxme
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/scsipi

LIB=	rumpdev_umass
IOCONFIG= UMASS.ioconf

.if ${MACHINE} == "i386"
SRCS=	umass.c umass_isdata.c umass_quirks.c umass_scsipi.c
SRCS+=	scsipiconf.c scsipi_base.c scsipi_ioctl.c scsi_base.c scsiconf.c
SRCS+=	atapi_base.c atapiconf.c

SRCS+=	cd.c sd.c

SRCS+=	sd_at_scsibus_at_umass.c
.else
SRCS=	dummy.c
.endif

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
