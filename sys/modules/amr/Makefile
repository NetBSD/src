#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:53 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	amr
IOCONF=	amr.ioconf

SRCS+=	amr.c

# -Wsign-compare issues
WARNS=	3

.include <bsd.kmodule.mk>
