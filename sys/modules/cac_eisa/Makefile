#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:50 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/eisa

KMOD=	cac_eisa
IOCONF=	cac_eisa.ioconf

SRCS+=	cac_eisa.c

.include <bsd.kmodule.mk>
