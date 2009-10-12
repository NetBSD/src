#PREFIX=/Users/agcrooks
PREFIX=/usr

LIB=luanetpgp
SRCS=glue.c
MKMAN=no
CPPFLAGS+=-g -I${PREFIX}/pkg/include
WARNS=4

.include <bsd.lib.mk>

OPSYS!= uname -s

.if ${OPSYS} == "Darwin"
.sinclude <bsd.warns.mk>

lib${LIB}.dylib:
	libtool -dynamic -o ${.TARGET} ${OBJS} ${PREFIX}/pkg/lib/liblua.dylib /usr/lib/libc.dylib ${PREFIX}/pkg/lib/libnetpgp.dylib

t: lib${LIB}.dylib
	cp Makefile a
	./netpgp.lua --sign --detached a
	./netpgp.lua --verify a.sig

.else
t:
	cp Makefile a
	./netpgp.lua --sign --detached a
	./netpgp.lua --verify a.sig
.endif
