#	$NetBSD: Makefile,v 1.1.2.1 2015/12/27 12:10:12 skrll Exp $
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
