#	$NetBSD: Makefile,v 1.2.38.1 2026/11/04 23:27:56 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio
IOCONF= virtio.ioconf

SRCS+=	virtio.c

CPPFLAGS+=	-DDIAGNOSTIC

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
