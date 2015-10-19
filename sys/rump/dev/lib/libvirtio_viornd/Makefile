#	$NetBSD: Makefile,v 1.2 2015/10/19 16:16:35 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_virtio_viornd
COMMENT=VirtIO entropy driver

IOCONF=	VIRTIO_VIORND.ioconf

SRCS=	viornd_at_virtio.c

SRCS+=	viornd.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
