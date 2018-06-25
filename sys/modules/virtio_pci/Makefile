#	$NetBSD: Makefile,v 1.1.2.2 2018/06/25 07:26:06 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio_pci
IOCONF=	virtio_pci.ioconf

SRCS+=	virtio_pci.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
