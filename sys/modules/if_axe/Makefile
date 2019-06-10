# $NetBSD: Makefile,v 1.2.48.1 2019/06/10 22:09:27 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axe
IOCONF=	if_axe.ioconf
SRCS=	if_axe.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
