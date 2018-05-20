#	$NetBSD: Makefile,v 1.1 2018/05/20 15:15:41 thorpej Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpiopps
IOCONF=		gpiopps.ioconf
SRCS=		gpiopps.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
