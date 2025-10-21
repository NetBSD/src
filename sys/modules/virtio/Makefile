#	$NetBSD: Makefile,v 1.3 2025/10/21 04:34:10 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio
IOCONF= virtio.ioconf

SRCS+=	virtio.c

CPPFLAGS+=	-DDIAGNOSTIC

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
