#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:06 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_nvme
IOCONF=	ld_nvme.ioconf

SRCS+=	ld_nvme.c

.include <bsd.kmodule.mk>
