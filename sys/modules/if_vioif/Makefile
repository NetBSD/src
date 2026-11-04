#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:54 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	if_vioif
IOCONF=	if_vioif.ioconf

SRCS+=	if_vioif.c

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
