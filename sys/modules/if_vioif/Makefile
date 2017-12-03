#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:56 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_vioif
IOCONF=	if_vioif.ioconf

SRCS+=	if_vioif.c

.include <bsd.kmodule.mk>
