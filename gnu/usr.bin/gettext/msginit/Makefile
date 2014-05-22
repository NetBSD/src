#	$NetBSD: Makefile,v 1.3.40.1 2014/05/22 11:36:30 yamt Exp $

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
