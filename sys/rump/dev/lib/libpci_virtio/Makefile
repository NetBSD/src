#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:34 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_virtio
COMMENT=VirtIO bus support

IOCONF=	PCI_VIRTIO.ioconf

SRCS=	virtio_at_pci.c

SRCS+=	virtio.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
