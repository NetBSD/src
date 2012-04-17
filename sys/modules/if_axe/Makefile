# $NetBSD: Makefile,v 1.1.2.1 2012/04/17 00:08:36 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axe
IOCONF=	if_axe.ioconf
SRCS=	if_axe.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
