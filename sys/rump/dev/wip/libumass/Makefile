#	$NetBSD: Makefile,v 1.3 2010/02/03 21:18:38 pooka Exp $
#
#
# umass.  includes scsibus+sd for simplicity due to the umass code
# needing compile time knowledge of its presence.  fixxxme
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/scsipi

LIB=	rumpdev_umass

SRCS=	umass.c umass_isdata.c umass_quirks.c umass_scsipi.c
SRCS+=	scsipiconf.c scsipi_base.c scsipi_ioctl.c scsi_base.c scsiconf.c sd.c
SRCS+=	atapi_base.c atapiconf.c

SRCS+=	sd_at_scsibus_at_umass.c

IOCONFIG= UMASS.ioconf

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
