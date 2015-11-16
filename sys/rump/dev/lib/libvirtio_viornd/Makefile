#	$NetBSD: Makefile,v 1.4 2015/11/16 23:27:08 pooka Exp $
#

RUMPTOP=${TOPRUMP}

.PATH:	${RUMPTOP}/../dev/pci

LIB=	rumpdev_virtio_viornd
COMMENT=VirtIO entropy driver

IOCONF=	VIRTIO_VIORND.ioconf
RUMP_COMPONENT=ioconf

SRCS+=	viornd.c

CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern
   
.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
