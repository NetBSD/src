#	$NetBSD: Makefile,v 1.5 2019/02/20 01:20:16 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

WARNS=	3

.include <bsd.kmodule.mk>
