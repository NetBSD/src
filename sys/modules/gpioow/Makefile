#	$NetBSD: Makefile,v 1.1 2011/08/31 12:46:37 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpioow
IOCONF=		gpioow.ioconf
SRCS=		gpioow.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
