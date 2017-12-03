#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:49 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	amr
IOCONF=	amr.ioconf

SRCS+=	amr.c

.include <bsd.kmodule.mk>
