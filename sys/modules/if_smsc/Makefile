# $NetBSD: Makefile,v 1.1.10.2 2014/08/20 00:04:33 tls Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_smsc
IOCONF=	if_smsc.ioconf
SRCS=	if_smsc.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
