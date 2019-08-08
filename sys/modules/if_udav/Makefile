# $NetBSD: Makefile,v 1.1 2019/08/08 16:06:14 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_udav
IOCONF=	if_udav.ioconf
SRCS=	if_udav.c

.include <bsd.kmodule.mk>
