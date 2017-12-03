#	$NetBSD: Makefile,v 1.1.24.2 2017/12/03 11:39:09 jdolecek Exp $
#

.PATH:	${.CURDIR}/../../../../dev/scsipi

LIB=	rumpdev_scsipi
COMMENT=SCSI & ATAPI mid-layer

IOCONF=	SCSIPI.ioconf

SRCS+=	scsipiconf.c scsipi_base.c scsipi_ioctl.c scsi_base.c scsiconf.c
SRCS+=	atapi_base.c atapiconf.c
SRCS+=	cd.c sd.c

.if 1
SRCS+=		scsipi_verbose.c
.endif

SRCS+=	scsipi_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
