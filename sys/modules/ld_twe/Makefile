#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:19 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_twe
IOCONF=	ld_twe.ioconf

SRCS+=	ld_twe.c

.include <bsd.kmodule.mk>
