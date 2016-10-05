#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:06 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_amr
IOCONF=	ld_amr.ioconf

SRCS+=	ld_amr.c

.include <bsd.kmodule.mk>
