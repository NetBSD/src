#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:29 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	amr
IOCONF=	amr.ioconf

SRCS+=	amr.c

# -Wsign-compare issues
WARNS=	3

.include <bsd.kmodule.mk>
