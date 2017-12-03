#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:57 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_virtio
IOCONF=	ld_virtio.ioconf

SRCS+=	ld_virtio.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
