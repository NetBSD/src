# $NetBSD: Makefile,v 1.1 2011/08/23 12:54:05 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axe
IOCONF=	if_axe.ioconf
SRCS=	if_axe.c

WARNS=	4

CPPFLAGS+=	-DINET -DNRND=1

.include <bsd.kmodule.mk>
