#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:48 rin Exp $

#.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	luareadhappy
SRCS=	luareadhappy.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

WARNS=	3

.include <bsd.kmodule.mk>
