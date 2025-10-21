#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:30 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_virtio
IOCONF=	ld_virtio.ioconf

SRCS+=	ld_virtio.c

CPPFLAGS+=	-DNBIO=1

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
