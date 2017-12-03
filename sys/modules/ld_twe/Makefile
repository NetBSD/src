#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:57 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_twe
IOCONF=	ld_twe.ioconf

SRCS+=	ld_twe.c

.include <bsd.kmodule.mk>
