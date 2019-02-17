# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:51 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_mue
IOCONF=	if_mue.ioconf
SRCS=	if_mue.c

.include <bsd.kmodule.mk>
