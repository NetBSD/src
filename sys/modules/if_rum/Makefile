# $NetBSD: Makefile,v 1.2 2011/08/23 12:55:50 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_rum
IOCONF=	if_rum.ioconf
SRCS=	if_rum.c

WARNS=	4

.include <bsd.kmodule.mk>
