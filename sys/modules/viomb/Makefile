#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:30 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	viomb
IOCONF=	viomb.ioconf

SRCS+=	viomb.c

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
