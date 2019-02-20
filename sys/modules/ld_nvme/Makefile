#	$NetBSD: Makefile,v 1.2 2019/02/20 00:55:03 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_nvme
IOCONF=	ld_nvme.ioconf

SRCS+=	ld_nvme.c

WARNS=	3

.include <bsd.kmodule.mk>
