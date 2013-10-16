#	$NetBSD: Makefile,v 1.1 2013/10/16 19:44:57 mbalmer Exp $

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

CPPFLAGS+=	-include ${.CURDIR}/luaconf.h \
		-I${S}/../external/mit/lua/dist/src

.include <bsd.kmodule.mk>
