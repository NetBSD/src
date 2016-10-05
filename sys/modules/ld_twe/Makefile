#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:07 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_twe
IOCONF=	ld_twe.ioconf

SRCS+=	ld_twe.c

.include <bsd.kmodule.mk>
