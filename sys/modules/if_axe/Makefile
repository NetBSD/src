# $NetBSD: Makefile,v 1.2 2012/02/02 19:43:07 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axe
IOCONF=	if_axe.ioconf
SRCS=	if_axe.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
