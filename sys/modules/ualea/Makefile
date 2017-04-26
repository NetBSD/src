#	$NetBSD: Makefile,v 1.1.4.2 2017/04/26 02:53:28 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	ualea
IOCONF=	ualea.ioconf

SRCS+=	ualea.c

.include <bsd.kmodule.mk>
