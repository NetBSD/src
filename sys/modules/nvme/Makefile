#	$NetBSD: Makefile,v 1.4.6.1 2019/06/10 22:09:35 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci ${S}/dev/ic

KMOD=	nvme
IOCONF=	nvme.ioconf
SRCS=	nvme.c nvme_pci.c

WARNS=	3

.include <bsd.kmodule.mk>
