#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:07 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/eisa

KMOD=	cac_eisa
IOCONF=	cac_eisa.ioconf

SRCS+=	cac_eisa.c

.include <bsd.kmodule.mk>
