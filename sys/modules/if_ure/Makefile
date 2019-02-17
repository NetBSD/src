# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:52 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_ure
IOCONF=	if_ure.ioconf
SRCS=	if_ure.c

.include <bsd.kmodule.mk>
