#	$NetBSD: Makefile,v 1.2 2025/10/21 04:35:29 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	cac
IOCONF=	cac.ioconf

SRCS+=	cac.c

CPPFLAGS+=	-DNBIO=1

WARNS=	3	# -Wsign-compare issues

.include <bsd.kmodule.mk>
