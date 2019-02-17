# $NetBSD: Makefile,v 1.3 2019/02/17 04:05:51 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axe
IOCONF=	if_axe.ioconf
SRCS=	if_axe.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
