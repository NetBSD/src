# $NetBSD: Makefile,v 1.1.2.2 2012/04/17 00:08:36 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_urtwn
IOCONF=	if_urtwn.ioconf
SRCS=	if_urtwn.c

WARNS=	4

.include <bsd.kmodule.mk>
