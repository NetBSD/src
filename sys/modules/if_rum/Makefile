# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:52 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_rum
IOCONF=	if_rum.ioconf
SRCS=	if_rum.c

WARNS=	3

.include <bsd.kmodule.mk>
