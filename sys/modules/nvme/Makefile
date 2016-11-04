#	$NetBSD: Makefile,v 1.2.4.2 2016/11/04 14:49:20 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

.include <bsd.kmodule.mk>
