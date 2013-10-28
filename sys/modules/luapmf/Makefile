#	$NetBSD: Makefile,v 1.1 2013/10/28 20:06:05 mbalmer Exp $

.include "../Makefile.inc"

KMOD=		luapmf
SRCS=		luapmf.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua

.include <bsd.kmodule.mk>
