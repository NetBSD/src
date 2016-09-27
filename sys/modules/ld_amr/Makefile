#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:07 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_amr
IOCONF=	ld_amr.ioconf

SRCS+=	ld_amr.c

.include <bsd.kmodule.mk>
