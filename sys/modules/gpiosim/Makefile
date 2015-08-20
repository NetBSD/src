#	$NetBSD: Makefile,v 1.2 2015/08/20 11:05:01 christos Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpiosim
IOCONF=		gpiosim.conf
SRCS=		gpiosim.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
