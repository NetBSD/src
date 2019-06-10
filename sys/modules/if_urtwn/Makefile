# $NetBSD: Makefile,v 1.1.44.1 2019/06/10 22:09:30 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_urtwn
IOCONF=	if_urtwn.ioconf
SRCS=	if_urtwn.c

WARNS=	3

.include <bsd.kmodule.mk>
