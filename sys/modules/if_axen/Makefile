# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:51 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axen
IOCONF=	if_axen.ioconf
SRCS=	if_axen.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
