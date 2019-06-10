# $NetBSD: Makefile,v 1.1.42.1 2019/06/10 22:09:28 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/usb

KMOD=	if_run
IOCONF=	if_run.ioconf
SRCS=	if_run.c

WARNS=	3

.include <bsd.kmodule.mk>
