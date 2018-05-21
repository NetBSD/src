# $NetBSD: Makefile,v 1.1.2.3 2018/05/21 04:35:49 pgoyette Exp $

.include <bsd.init.mk>
.if ${MKPIC} != no
LUA_MODULES=netpgp
LUA_SRCS.netpgp=glue.c
LUA_DPLIBS+=netpgp ${.CURDIR}/../../lib/netpgp

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/bindings/lua
.endif

.include <bsd.lua.mk>
