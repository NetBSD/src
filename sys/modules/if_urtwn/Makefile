# $NetBSD: Makefile,v 1.1.4.2 2012/06/14 09:48:48 sborrill Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_urtwn
IOCONF=	if_urtwn.ioconf
SRCS=	if_urtwn.c

WARNS=	4

.include <bsd.kmodule.mk>
