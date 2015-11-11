#	$NetBSD: Makefile,v 1.2 2015/11/11 21:52:47 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_virtio_vioscsi
COMMENT=VirtIO SCSI driver

IOCONF=	VIRTIO_VIOSCSI.ioconf

COMPONENT_SIMPLE= virtio_vioscsi

SRCS+=	vioscsi.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
