#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:06 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	amr
IOCONF=	amr.ioconf

SRCS+=	amr.c

.include <bsd.kmodule.mk>
