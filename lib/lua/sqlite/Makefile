#	$NetBSD: Makefile,v 1.2 2011/10/16 00:45:54 mbalmer Exp $

LUA_MODULES=		sqlite
LUA_SRCS.sqlite=	sqlite.c

LUA_DPLIBS+=		sqlite3 \
			${.CURDIR}/../../../external/public-domain/sqlite/lib
.include <bsd.lua.mk>
