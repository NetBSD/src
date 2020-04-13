#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:13 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_url
IOCONF=	if_url.ioconf
SRCS=	if_url.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
