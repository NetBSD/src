#	$NetBSD: Makefile,v 1.2.2.1 2014/08/10 06:56:10 tls Exp $

.include "../Makefile.inc"

KMOD=		luapmf
SRCS=		luapmf.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src

.include <bsd.kmodule.mk>
