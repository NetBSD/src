# $NetBSD: Makefile,v 1.1.36.1 2019/06/10 22:09:29 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_smsc
IOCONF=	if_smsc.ioconf
SRCS=	if_smsc.c

CPPFLAGS+=	-DINET

.include <bsd.kmodule.mk>
