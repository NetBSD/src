#	$NetBSD: Makefile,v 1.1.2.2 2018/05/21 04:35:56 pgoyette Exp $

LUA_MODULES=		bozohttpd
LUA_SRCS.bozohttpd=	glue.c
LUA_DPLIBS+=		bozohttpd \
			${.CURDIR}/../../../libexec/httpd/libbozohttpd

EXTDIST= 		${.CURDIR}/../../../libexec/httpd

CPPFLAGS+= 		-I${EXTDIST}

.PATH: ${EXTDIST}/lua

.include <bsd.lua.mk>
