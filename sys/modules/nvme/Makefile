#	$NetBSD: Makefile,v 1.1 2016/09/16 11:35:07 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic ${S}/dev

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

# move to separate module?
SRCS+=	ld_nvme.c ld.c

.include <bsd.kmodule.mk>
