#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:12 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_upl
IOCONF=	if_upl.ioconf
SRCS=	if_upl.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
