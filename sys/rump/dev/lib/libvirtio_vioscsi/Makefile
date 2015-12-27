#	$NetBSD: Makefile,v 1.3.2.2 2015/12/27 12:10:12 skrll Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_virtio_vioscsi
COMMENT=VirtIO SCSI driver

IOCONF=	VIRTIO_VIOSCSI.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	vioscsi.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
