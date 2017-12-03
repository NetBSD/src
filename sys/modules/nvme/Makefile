#	$NetBSD: Makefile,v 1.4 2017/12/03 11:49:32 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

.include <bsd.kmodule.mk>
