#	$NetBSD: Makefile,v 1.1.2.2 2017/04/19 00:01:39 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	ualea
IOCONF=	ualea.ioconf

SRCS+=	ualea.c

.include <bsd.kmodule.mk>
