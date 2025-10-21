#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:30 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio_pci
IOCONF=	virtio_pci.ioconf

SRCS+=	virtio_pci.c

CPPFLAGS+=	-DDIAGNOSTIC

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
