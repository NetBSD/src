#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:08 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	ld_twa
IOCONF=	ld_twa.ioconf

SRCS+=	ld_twa.c

.include <bsd.kmodule.mk>
