#	$NetBSD: Makefile,v 1.1 2019/09/17 04:38:09 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_urndis
IOCONF=	if_urndis.ioconf
SRCS=	if_urndis.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
