#	$NetBSD: Makefile,v 1.1 2019/09/17 04:38:08 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_kue
IOCONF=	if_kue.ioconf
SRCS=	if_kue.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
