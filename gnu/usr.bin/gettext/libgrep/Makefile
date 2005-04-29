#	$NetBSD: Makefile,v 1.2 2005/04/29 15:05:01 christos Exp $

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

.include <bsd.lib.mk>
