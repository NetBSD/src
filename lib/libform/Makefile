#	$NetBSD: Makefile,v 1.3 2001/02/10 14:53:11 blymn Exp $
#

CPPFLAGS+=-I${.CURDIR}
.if defined(DEBUG_FORMS)
CFLAGS+=-g
CPPFLAGS+=-DDEBUG
.endif
LIB=	form
SRCS=	driver.c field_types.c internals.c field.c form.c post.c type_alnum.c \
        type_alpha.c type_integer.c type_numeric.c type_enum.c type_regex.c \
	type_ipv4.c type_ipv6.c

INCS=	form.h
INCSDIR=/usr/include
LDFLAGS += -g

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
