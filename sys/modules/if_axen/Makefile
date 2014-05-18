# $NetBSD: Makefile,v 1.1.4.2 2014/05/18 17:46:11 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axen
IOCONF=	if_axen.ioconf
SRCS=	if_axen.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
