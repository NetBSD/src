#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:30 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vioscsi
IOCONF=	vioscsi.ioconf

SRCS+=	vioscsi.c

DBG+=	-DDEBUG -DDIAGNOSTIC -DLOCKDEBUG

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
