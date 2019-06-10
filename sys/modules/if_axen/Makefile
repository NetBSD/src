# $NetBSD: Makefile,v 1.1.36.1 2019/06/10 22:09:27 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_axen
IOCONF=	if_axen.ioconf
SRCS=	if_axen.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
