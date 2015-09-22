#	$NetBSD: Makefile,v 1.2.6.1 2015/09/22 12:06:13 skrll Exp $
#

.PATH:	${.CURDIR}/../../../../dev/scsipi

LIB=	rumpdev_scsipi
IOCONF=	SCSIPI.ioconf

SRCS+=	scsipiconf.c scsipi_base.c scsipi_ioctl.c scsi_base.c scsiconf.c
SRCS+=	atapi_base.c atapiconf.c
SRCS+=	cd.c sd.c

.if 1
SRCS+=		scsipi_verbose.c
.endif

SRCS+=	scsipi_component.c

CPPFLAGS+=	-I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
