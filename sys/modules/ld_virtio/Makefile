#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_virtio
IOCONF=	ld_virtio.ioconf

SRCS+=	ld_virtio.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
