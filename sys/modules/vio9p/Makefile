#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:14 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vio9p
IOCONF=	vio9p.ioconf

SRCS+=	vio9p.c

.include <bsd.kmodule.mk>
