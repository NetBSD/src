#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:57 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_nvme
IOCONF=	ld_nvme.ioconf

SRCS+=	ld_nvme.c

.include <bsd.kmodule.mk>
