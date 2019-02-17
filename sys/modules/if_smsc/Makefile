# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:52 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_smsc
IOCONF=	if_smsc.ioconf
SRCS=	if_smsc.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
