#	$NetBSD: Makefile,v 1.1.58.1 2026/11/04 23:27:54 perseant Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	cac
IOCONF=	cac.ioconf

SRCS+=	cac.c

CPPFLAGS+=	-DNBIO=1

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
