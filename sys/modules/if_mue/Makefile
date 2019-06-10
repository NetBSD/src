# $NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:28 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_mue
IOCONF=	if_mue.ioconf
SRCS=	if_mue.c

.include <bsd.kmodule.mk>
