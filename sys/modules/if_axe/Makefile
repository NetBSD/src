# $NetBSD: Makefile,v 1.1.6.1 2012/02/18 07:35:37 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axe
IOCONF=	if_axe.ioconf
SRCS=	if_axe.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
