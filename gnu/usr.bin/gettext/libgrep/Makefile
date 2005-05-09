#	$NetBSD: Makefile,v 1.3 2005/05/09 17:19:06 jmc Exp $

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

.if ${MACHINE_ARCH} == "vax"
depend dependall realdepend realall: stdbool.h

stdbool.h:
	sed -e 's:@HAVE__BOOL@:0:g' \
		${DIST}/gettext/gettext-tools/lib/stdbool_.h > stdbool.h

CLEANFILES+=stdbool.h
CPPFLAGS+=-I.
.endif

.include <bsd.lib.mk>
