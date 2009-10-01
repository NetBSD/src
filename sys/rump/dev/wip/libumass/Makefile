#	$NetBSD: Makefile,v 1.1 2009/10/01 21:46:30 pooka Exp $
#
#
# umass.  includes scsibus+sd for simplicity due to the umass code
# needing compile time knowledge of its presence.  fixxxme
#

.PATH:	${.CURDIR}/../../../../dev/usb ${.CURDIR}/../../../../dev/scsipi

LIB=	rumpdev_umass

SRCS=	umass.c umass_isdata.c umass_quirks.c umass_scsipi.c
SRCS+=	scsipiconf.c scsipi_base.c scsipi_ioctl.c scsi_base.c scsiconf.c sd.c

SRCS+=	sd_at_scsibus_at_umass.c

CFLAGS+=	-Wno-pointer-sign
CPPFLAGS+=	-I${.CURDIR}/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
