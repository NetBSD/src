#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	cac_pci
IOCONF=	cac_pci.ioconf

SRCS+=	cac_pci.c

.include <bsd.kmodule.mk>
