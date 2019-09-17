#	$NetBSD: Makefile,v 1.1 2019/09/17 04:38:08 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_url
IOCONF=	if_url.ioconf
SRCS=	if_url.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
