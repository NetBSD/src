#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	cac
IOCONF=	cac.ioconf

SRCS+=	cac.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
