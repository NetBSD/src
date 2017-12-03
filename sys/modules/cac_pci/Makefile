#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:50 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	cac_pci
IOCONF=	cac_pci.ioconf

SRCS+=	cac_pci.c

.include <bsd.kmodule.mk>
