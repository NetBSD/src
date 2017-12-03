#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:56 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_amr
IOCONF=	ld_amr.ioconf

SRCS+=	ld_amr.c

.include <bsd.kmodule.mk>
