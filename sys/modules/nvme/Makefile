#	$NetBSD: Makefile,v 1.2 2016/09/27 03:53:09 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

.include <bsd.kmodule.mk>
