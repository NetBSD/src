#	$NetBSD: Makefile,v 1.1 2011/10/02 11:43:30 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpioiic
IOCONF=		gpioiic.ioconf
SRCS=		gpioiic.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
