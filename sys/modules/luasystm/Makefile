#	$NetBSD: Makefile,v 1.2.4.2 2014/08/20 00:04:34 tls Exp $

.include "../Makefile.inc"

KMOD=		luasystm
SRCS=		luasystm.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src

.include <bsd.kmodule.mk>
