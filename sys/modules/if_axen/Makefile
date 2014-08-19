# $NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:33 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axen
IOCONF=	if_axen.ioconf
SRCS=	if_axen.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
