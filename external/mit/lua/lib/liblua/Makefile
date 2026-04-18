# $NetBSD: Makefile,v 1.15 2026/04/18 21:23:44 rillig Exp $

LIB=		lua
LUA_CORE=	lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c \
		llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c \
		lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c
LUA_LIB=	lauxlib.c lbaselib.c lcorolib.c ldblib.c liolib.c \
		lmathlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c \
		linit.c
SRCS=		${LUA_CORE} ${LUA_LIB}

.include <bsd.own.mk>

INCS=		lauxlib.h lua.h lua.hpp luaconf.h lualib.h
INCSDIR=	/usr/include

CPPFLAGS+=	-DLUA_USE_POSIX -DLUA_USE_DLOPEN
CWARNFLAGS.clang+=	-Wno-empty-body -Wno-error=string-plus-int
COPTS.lstring.c+=	${CC_WNO_STRINGOP_OVERFLOW}
LINTFLAGS+=	-X 141		# '4294967295 << 8' overflows 'unsigned int'
LINTFLAGS+=	-X 247		# pointer cast between unrelated types

LIBDPLIBS+=	m		${NETBSDSRCDIR}/lib/libm

FILES+=			lua.pc
FILESOWN_lua.pc=	${BINOWN}
FILESGRP_lua.pc=	${BINGRP}
FILESMODE_lua.pc=	${NONBINMODE}
FILESDIR_lua.pc=	/usr/lib/pkgconfig

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src

.include <bsd.lib.mk>
