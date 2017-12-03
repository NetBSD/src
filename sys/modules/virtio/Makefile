#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:01 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio
IOCONF=	virtio.ioconf

SRCS+=	virtio.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
