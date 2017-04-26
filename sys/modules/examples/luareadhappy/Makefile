#	$NetBSD: Makefile,v 1.1.4.2 2017/04/26 02:53:28 pgoyette Exp $

#.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	luareadhappy
SRCS=	luareadhappy.c

CPPFLAGS+=	-I${S}/../external/mit/lua/dist/src \
		-I${S}/modules/lua \
		-I${S}/sys

.include <bsd.kmodule.mk>
