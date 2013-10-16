#	$NetBSD: Makefile,v 1.1 2013/10/16 19:47:19 mbalmer Exp $

.include "../Makefile.inc"

KMOD=		luacore
SRCS=		luacore.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua

.include <bsd.kmodule.mk>
