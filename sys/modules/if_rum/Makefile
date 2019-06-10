# $NetBSD: Makefile,v 1.2.54.1 2019/06/10 22:09:28 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_rum
IOCONF=	if_rum.ioconf
SRCS=	if_rum.c

WARNS=	3

.include <bsd.kmodule.mk>
