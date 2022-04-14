#	$NetBSD: Makefile,v 1.6 2022/04/14 16:50:27 pgoyette Exp $
#

.PATH:	${.CURDIR}/../../../../dev/scsipi

LIB=	rumpdev_scsipi
COMMENT=SCSI & ATAPI mid-layer

IOCONF=	SCSIPI.ioconf

SRCS+=	scsipiconf.c scsipi_base.c scsipi_ioctl.c scsi_base.c scsiconf.c
SRCS+=	scsi_subr.c atapi_base.c atapiconf.c
SRCS+=	cd.c sd.c

.if 1
SRCS+=		scsipi_verbose.c
.endif

SRCS+=	scsipi_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
