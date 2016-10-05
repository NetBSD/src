#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:04 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	cac_pci
IOCONF=	cac_pci.ioconf

SRCS+=	cac_pci.c

.include <bsd.kmodule.mk>
