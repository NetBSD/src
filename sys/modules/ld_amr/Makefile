#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:19 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_amr
IOCONF=	ld_amr.ioconf

SRCS+=	ld_amr.c

.include <bsd.kmodule.mk>
