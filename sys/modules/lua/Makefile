#	$NetBSD: Makefile,v 1.6.4.2 2014/05/18 17:46:11 rmind Exp $

.include "../Makefile.inc"

.PATH:		${S}/../external/mit/lua/dist/src \
		${S}/../lib/libc/string

KMOD=		lua
SRCS=		lua.c

WARNS=		1

# Lua core source code
SRCS+=		lapi.c \
		lcode.c \
		ldebug.c \
		ldo.c \
		ldump.c \
		lfunc.c \
		lgc.c \
		llex.c \
		lmem.c \
		lobject.c \
		lopcodes.c \
		lparser.c \
		lstate.c \
		lstring.c \
		ltable.c \
		ltm.c \
		lundump.c \
		lvm.c \
		lzio.c

# Lua auxlib source code
SRCS+=		lauxlib.c

# Lua stdlibs source code
SRCS+=		lbaselib.c \
		lstrlib.c \
		ltablib.c \
		linit.c

# Compatability code
SRCS+=		strcspn.c \
		strpbrk.c \
		strspn.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/sys

.include <bsd.kmodule.mk>
