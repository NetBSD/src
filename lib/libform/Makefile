#	$NetBSD: Makefile,v 1.1 2000/12/17 12:04:30 blymn Exp $
#

DBG=-g
CPPFLAGS+=-I${.CURDIR} #-DDEBUG
LIB=	form
SRCS=	driver.c field_types.c internals.c field.c form.c post.c type_alnum.c \
        type_alpha.c type_integer.c type_numeric.c type_enum.c type_regex.c \
	type_ipv4.c

INCS=	form.h
INCSDIR=/usr/include
LDFLAGS += -g

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
