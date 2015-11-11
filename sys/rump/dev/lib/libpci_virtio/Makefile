#	$NetBSD: Makefile,v 1.3 2015/11/11 21:52:46 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_virtio
COMMENT=VirtIO bus support

IOCONF=	PCI_VIRTIO.ioconf
COMPONENT_SIMPLE= pci_virtio

SRCS+=	virtio.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
