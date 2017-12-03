#	$NetBSD: Makefile,v 1.1.16.2 2017/12/03 11:39:01 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vioscsi
IOCONF=	vioscsi.ioconf

SRCS+=	vioscsi.c

DBG+=	-DDEBUG -DDIAGNOSTIC -DLOCKDEBUG

.include <bsd.kmodule.mk>
