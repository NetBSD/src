#	$NetBSD: Makefile,v 1.1.12.2 2017/08/28 17:53:11 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vioscsi
IOCONF=	vioscsi.ioconf

SRCS+=	vioscsi.c

DBG+=	-DDEBUG -DDIAGNOSTIC -DLOCKDEBUG

.include <bsd.kmodule.mk>
