#	$NetBSD: Makefile,v 1.1.20.1 2019/06/10 22:09:24 christos Exp $

#.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	luareadhappy
SRCS=	luareadhappy.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

WARNS=	3

.include <bsd.kmodule.mk>
