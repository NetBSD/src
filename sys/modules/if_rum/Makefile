# $NetBSD: Makefile,v 1.1 2011/08/23 12:54:05 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_rum
IOCONF=	if_rum.ioconf
SRCS=	if_rum.c

WARNS=	4
#CWARNFLAGS+=	-Wno-sign-compare

.include <bsd.kmodule.mk>
