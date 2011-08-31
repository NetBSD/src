#	$NetBSD: Makefile,v 1.2 2011/08/31 14:42:18 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpio
IOCONF=		gpio.ioconf
SRCS=		gpio.c

CPPFLAGS+=	-I${S}/gpio
CWARNFLAGS+=	-Wno-unused

.include <bsd.kmodule.mk>
