#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:07 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	cac
IOCONF=	cac.ioconf

SRCS+=	cac.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
