#	$NetBSD: Makefile,v 1.5 2006/11/10 17:36:08 christos Exp $

MKPRIVATELIB=	yes

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
