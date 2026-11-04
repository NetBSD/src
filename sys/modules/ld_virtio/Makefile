#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:55 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_virtio
IOCONF=	ld_virtio.ioconf

SRCS+=	ld_virtio.c

CPPFLAGS+=	-DNBIO=1

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
