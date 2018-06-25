#	$NetBSD: Makefile,v 1.1.20.1 2018/06/25 07:26:06 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	virtio

SRCS+=	virtio.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
