# $NetBSD: Makefile,v 1.1 2012/08/04 04:37:26 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	uatp
IOCONF=	uatp.ioconf
SRCS=	uatp.c

WARNS=	4

.include <bsd.kmodule.mk>
