#	$NetBSD: Makefile,v 1.1.2.2 2010/10/09 03:32:40 yamt Exp $
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

SRCS+=	component.c

CPPFLAGS+=	-I${.CURDIR}/opt -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
