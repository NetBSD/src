#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:17 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	amr
IOCONF=	amr.ioconf

SRCS+=	amr.c

.include <bsd.kmodule.mk>
