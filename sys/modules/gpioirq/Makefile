#	$NetBSD: Makefile,v 1.1 2018/05/19 14:15:40 thorpej Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpioirq
IOCONF=		gpioirq.ioconf
SRCS=		gpioirq.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
