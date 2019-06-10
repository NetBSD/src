# $NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:30 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_ure
IOCONF=	if_ure.ioconf
SRCS=	if_ure.c

.include <bsd.kmodule.mk>
