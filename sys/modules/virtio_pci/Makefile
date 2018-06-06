#	$NetBSD: Makefile,v 1.1 2018/06/06 16:11:36 jakllsch Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio_pci
IOCONF=	virtio_pci.ioconf

SRCS+=	virtio_pci.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
