#	$NetBSD: Makefile,v 1.1.34.1 2026/11/04 23:27:56 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vio9p
IOCONF=	vio9p.ioconf

SRCS+=	vio9p.c

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
