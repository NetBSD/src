# $NetBSD: Makefile,v 1.1 2019/02/06 11:55:06 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_ure
IOCONF=	if_ure.ioconf
SRCS=	if_ure.c

WARNS=	5

.include <bsd.kmodule.mk>
