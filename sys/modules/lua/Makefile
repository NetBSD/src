#	$NetBSD: Makefile,v 1.2 2013/10/31 16:50:14 mbalmer Exp $

.include "../Makefile.inc"

.PATH:		${S}/../external/mit/lua/dist/src \
		${S}/../lib/libc/string

KMOD=		lua
SRCS=		lua.c

WARNS=		1

# Lua source code
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
		lzio.c \
		# lauxlib.c \
		ltablib.c \
		lstrlib.c \
		lbaselib.c \
		linit.c

# Compatability code
SRCS+=		strcspn.c \
		strncat.c \
		strpbrk.c \
		strspn.c

CFLAGS+=	-include ${.CURDIR}/luaconf.h

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src

.include <bsd.kmodule.mk>
