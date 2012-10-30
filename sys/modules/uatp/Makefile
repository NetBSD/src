# $NetBSD: Makefile,v 1.1.4.2 2012/10/30 17:22:42 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	uatp
IOCONF=	uatp.ioconf
SRCS=	uatp.c

WARNS=	4

.include <bsd.kmodule.mk>
