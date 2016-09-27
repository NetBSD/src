#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:07 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_vioif
IOCONF=	if_vioif.ioconf

SRCS+=	if_vioif.c

.include <bsd.kmodule.mk>
