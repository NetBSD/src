#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_vioif
IOCONF=	if_vioif.ioconf

SRCS+=	if_vioif.c

.include <bsd.kmodule.mk>
