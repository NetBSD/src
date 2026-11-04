#	$NetBSD: Makefile,v 1.1.56.1 2026/11/04 23:27:56 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vioscsi
IOCONF=	vioscsi.ioconf

SRCS+=	vioscsi.c

DBG+=	-DDEBUG -DDIAGNOSTIC -DLOCKDEBUG

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
