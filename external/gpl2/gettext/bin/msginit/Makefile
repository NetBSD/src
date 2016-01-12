#	$NetBSD: Makefile,v 1.1 2016/01/12 22:58:02 christos Exp $

PROG=	msginit
SRCS+=	msginit.c \
localealias.c \
localename.c

.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=-I${DIST}/gettext-runtime/intl
CPPFLAGS+=-DLIBDIR=\"/usr/share\" \
    -DPROJECTSDIR=\"/usr/share\" \
    -DLOCALE_ALIAS_PATH=\"/usr/share/locale\"

COPTS.msginit.c = -Wno-stack-protector
COPTS.localealias.c = -Wno-stack-protector

.PATH: ${DIST}/gettext-runtime/intl
