#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:09 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio
IOCONF=	virtio.ioconf

SRCS+=	virtio.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
