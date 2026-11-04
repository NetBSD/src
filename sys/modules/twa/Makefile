#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:56 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	twa
IOCONF=	twa.ioconf

SRCS+=	twa.c

CPPFLAGS+=	-DDIAGNOSTIC

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
