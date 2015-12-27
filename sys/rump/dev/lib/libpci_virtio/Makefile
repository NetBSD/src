#	$NetBSD: Makefile,v 1.1.2.1 2015/12/27 12:10:10 skrll Exp $
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
