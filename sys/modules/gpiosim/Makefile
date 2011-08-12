#	$NetBSD: Makefile,v 1.1 2011/08/12 08:00:53 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/dev/gpio

KMOD=   	gpiosim
SRCS=		gpiosim.c

CPPFLAGS+=	-I${S}/gpio

.include <bsd.kmodule.mk>
