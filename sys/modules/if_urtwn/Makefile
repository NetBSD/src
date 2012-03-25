# $NetBSD: Makefile,v 1.1 2012/03/25 00:11:16 nonaka Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_urtwn
IOCONF=	if_urtwn.ioconf
SRCS=	if_urtwn.c

WARNS=	4

.include <bsd.kmodule.mk>
