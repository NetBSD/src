# $NetBSD: Makefile,v 1.1.6.2 2014/05/22 11:41:07 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_smsc
IOCONF=	if_smsc.ioconf
SRCS=	if_smsc.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
