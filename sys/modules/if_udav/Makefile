# $NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:12 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_udav
IOCONF=	if_udav.ioconf
SRCS=	if_udav.c

.include <bsd.kmodule.mk>
