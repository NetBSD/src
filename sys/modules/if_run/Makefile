# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:52 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_run
IOCONF=	if_run.ioconf
SRCS=	if_run.c

WARNS=	3

.include <bsd.kmodule.mk>
