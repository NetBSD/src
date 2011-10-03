#	$NetBSD: Makefile,v 1.3 2011/10/03 23:39:05 jmcneill Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpio
IOCONF=		gpio.ioconf
SRCS=		gpio.c

CWARNFLAGS+=	-Wno-unused

.include <bsd.kmodule.mk>
