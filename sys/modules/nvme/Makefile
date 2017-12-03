#	$NetBSD: Makefile,v 1.3 2017/12/03 11:38:58 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic

CFLAGS+=	-DNVME_DEBUG -DDEBUG -DDIAGNOSTIC -DLOCKDEBUG

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

.include <bsd.kmodule.mk>
