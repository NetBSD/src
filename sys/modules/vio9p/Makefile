#	$NetBSD: Makefile,v 1.1 2019/10/28 02:56:40 ozaki-r Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vio9p
IOCONF=	vio9p.ioconf

SRCS+=	vio9p.c

.include <bsd.kmodule.mk>
