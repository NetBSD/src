#	$NetBSD: Makefile,v 1.6.2.1 2014/08/10 06:56:10 tls Exp $

.include "../Makefile.inc"

.PATH:		${S}/../external/mit/lua/dist/src \
		${S}/../lib/libc/string

KMOD=		lua
SRCS=		lua.c

WARNS=		0

# Lua core source code
SRCS+=		lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c \
		llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c \
		lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c

# Lua stdlibs source code
SRCS+=		lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c \
		lstrlib.c ltablib.c lutf8lib.c linit.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/sys

.include <bsd.kmodule.mk>
