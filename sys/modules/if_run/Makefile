# $NetBSD: Makefile,v 1.1 2012/05/30 14:30:35 nonaka Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_run
IOCONF=	if_run.ioconf
SRCS=	if_run.c

WARNS=	4

.include <bsd.kmodule.mk>
