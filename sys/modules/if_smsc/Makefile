# $NetBSD: Makefile,v 1.1.4.2 2014/05/18 17:46:11 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_smsc
IOCONF=	if_smsc.ioconf
SRCS=	if_smsc.c

WARNS=	4

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
