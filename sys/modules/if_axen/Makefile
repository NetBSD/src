# $NetBSD: Makefile,v 1.1 2013/10/26 09:16:20 nonaka Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axen
IOCONF=	if_axen.ioconf
SRCS=	if_axen.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
