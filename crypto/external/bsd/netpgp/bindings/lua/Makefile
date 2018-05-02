# $NetBSD: Makefile,v 1.1.2.2 2018/05/02 07:19:58 pgoyette Exp $

LUA_MODULES=netpgp
LUA_SRCS.netpgp=glue.c
LUA_DPLIBS+=netpgp ${.CURDIR}/../../lib/netpgp

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/bindings/lua

.include <bsd.lua.mk>
