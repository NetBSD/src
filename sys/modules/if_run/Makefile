# $NetBSD: Makefile,v 1.1.4.2 2012/10/30 17:22:41 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_run
IOCONF=	if_run.ioconf
SRCS=	if_run.c

WARNS=	4

.include <bsd.kmodule.mk>
