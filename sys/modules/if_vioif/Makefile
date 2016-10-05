#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:06 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_vioif
IOCONF=	if_vioif.ioconf

SRCS+=	if_vioif.c

.include <bsd.kmodule.mk>
