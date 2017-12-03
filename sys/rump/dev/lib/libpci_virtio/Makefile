#	$NetBSD: Makefile,v 1.4.16.2 2017/12/03 11:39:08 jdolecek Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_pci_virtio
COMMENT=VirtIO bus support

IOCONF=	PCI_VIRTIO.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	virtio.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
