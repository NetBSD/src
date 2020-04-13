#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:12 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_cdce
IOCONF=	if_cdce.ioconf
SRCS=	if_cdce.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
