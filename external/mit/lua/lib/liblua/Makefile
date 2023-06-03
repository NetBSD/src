# $NetBSD: Makefile,v 1.13 2023/06/03 09:09:07 lukem Exp $

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
LINTFLAGS+=	-X 141		# integer overflow detected, op '<<'
LINTFLAGS+=	-X 247		# pointer cast from 'pointer to struct
				# GCObject' to 'pointer to union GCUnion' may
				# be troublesome

LIBDPLIBS+=	m		${NETBSDSRCDIR}/lib/libm

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src

.include <bsd.lib.mk>
