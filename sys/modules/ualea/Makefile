#	$NetBSD: Makefile,v 1.1.16.2 2017/12/03 11:39:01 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	ualea
IOCONF=	ualea.ioconf

SRCS+=	ualea.c

.include <bsd.kmodule.mk>
