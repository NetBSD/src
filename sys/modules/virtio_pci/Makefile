#	$NetBSD: Makefile,v 1.1.40.1 2026/11/04 23:27:56 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio_pci
IOCONF=	virtio_pci.ioconf

SRCS+=	virtio_pci.c

CPPFLAGS+=	-DDIAGNOSTIC

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
