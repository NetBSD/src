# $NetBSD: Makefile,v 1.1 2018/08/25 20:12:22 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_mue
IOCONF=	if_mue.ioconf
SRCS=	if_mue.c

WARNS=	5

.include <bsd.kmodule.mk>
