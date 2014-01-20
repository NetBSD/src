#	$NetBSD: Makefile,v 1.4 2014/01/20 10:57:38 apb Exp $

PROG=	msginit
SRCS+=	msginit.c \
localealias.c \
localename.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=-I${DIST}/gettext/gettext-runtime/intl
CPPFLAGS+=-DLIBDIR=\"/usr/share\" \
    -DPROJECTSDIR=\"/usr/share\" \
    -DLOCALE_ALIAS_PATH=\"/usr/share/locale\"

COPTS.msginit.c = -Wno-stack-protector
COPTS.localealias.c = -Wno-stack-protector

.PATH: ${DIST}/gettext/gettext-runtime/intl
