#	$NetBSD: Makefile,v 1.2.2.2 2016/10/05 20:56:07 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

.include <bsd.kmodule.mk>
