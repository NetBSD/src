#	$NetBSD: Makefile,v 1.1.30.1 2015/09/22 12:06:09 skrll Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpiosim
IOCONF=		gpiosim.conf
SRCS=		gpiosim.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
