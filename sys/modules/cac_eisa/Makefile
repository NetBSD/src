#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:18 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/eisa

KMOD=	cac_eisa
IOCONF=	cac_eisa.ioconf

SRCS+=	cac_eisa.c

.include <bsd.kmodule.mk>
