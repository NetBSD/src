# $NetBSD: Makefile,v 1.1 2010/10/31 11:26:01 mbalmer Exp $

LIB=		lua
SRCS=		lapi.c lauxlib.c lbaselib.c lcode.c ldblib.c ldebug.c ldo.c \
		ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c \
		lmem.c loadlib.c lobject.c lopcodes.c loslib.c lparser.c \
		lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c \
		lundump.c lvm.c lzio.c

.include <bsd.own.mk>

INCS=		lauxlib.h lua.h luaconf.h lualib.h
INCSDIR=	/usr/include

CFLAGS=		-DLUA_USE_POSIX -DLUA_USE_DLOPEN

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src

.include <bsd.lib.mk>
