#	$NetBSD: Makefile,v 1.3 2015/11/11 21:52:47 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_virtio_viornd
COMMENT=VirtIO entropy driver

IOCONF=	VIRTIO_VIORND.ioconf
COMPONENT_SIMPLE= virtio_viornd

SRCS+=	viornd.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
