#	$NetBSD: Makefile,v 1.1.2.2 2017/04/15 04:27:31 kamil Exp $

#.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	luareadhappy
SRCS=	luareadhappy.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

.include <bsd.kmodule.mk>
