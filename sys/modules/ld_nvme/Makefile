#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_nvme
IOCONF=	ld_nvme.ioconf

SRCS+=	ld_nvme.c

.include <bsd.kmodule.mk>
