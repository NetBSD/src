#	$NetBSD: Makefile,v 1.1 2026/11/18 18:04:25 brad Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/onewire

KMOD=   	ds2413gpio
IOCONF=		ds2413gpio.ioconf
SRCS=		ds2413gpio.c

CPPFLAGS+=	-I${S}/onewire

WARNS=		3

.include <bsd.kmodule.mk>
