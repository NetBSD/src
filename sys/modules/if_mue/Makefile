# $NetBSD: Makefile,v 1.1.2.2 2018/09/06 06:56:43 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_mue
IOCONF=	if_mue.ioconf
SRCS=	if_mue.c

WARNS=	5

.include <bsd.kmodule.mk>
