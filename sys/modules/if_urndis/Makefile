#	$NetBSD: Makefile,v 1.1.8.2 2020/04/13 08:05:13 martin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_urndis
IOCONF=	if_urndis.ioconf
SRCS=	if_urndis.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
