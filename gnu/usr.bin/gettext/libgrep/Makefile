#	$NetBSD: Makefile,v 1.6 2008/05/03 14:48:30 lukem Exp $

LIBISPRIVATE=	yes

.include <bsd.own.mk>

LIB=	grep
SRCS= \
dfa.c \
hard-locale.c \
kwset.c \
m-common.c \
m-fgrep.c \
m-regex.c \
memchr.c \
regex.c \
strdup.c 

.include "${.CURDIR}/../Makefile.inc"

.PATH:	${DIST}/gettext/gettext-tools/libgrep 
CPPFLAGS+= -I${DIST}/gettext/gettext-tools/libgrep

COPTS.regex.c = -Wno-stack-protector

.include <bsd.lib.mk>
