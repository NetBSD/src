# $NetBSD: Makefile,v 1.1 2013/11/16 16:59:04 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_smsc
IOCONF=	if_smsc.ioconf
SRCS=	if_smsc.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
