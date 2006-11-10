#	$NetBSD: Makefile,v 1.3 2006/11/10 17:36:08 christos Exp $

PROG=	msginit
SRCS+=	msginit.c \
localealias.c \
localename.c

.include <bsd.prog.mk>
.include "${.CURDIR}/../Makefile.inc.prog"

CPPFLAGS+=-I${DIST}/gettext/gettext-runtime/intl
CPPFLAGS+=-DLIBDIR=\"${DESTDIR}/usr/share\" \
    -DPROJECTSDIR=\"${DESTDIR}/usr/share\" \
    -DLOCALE_ALIAS_PATH=\"${DESTDIR}/usr/share/locale\"

COPTS.msginit.c = -Wno-stack-protector
COPTS.localealias.c = -Wno-stack-protector

.PATH: ${DIST}/gettext/gettext-runtime/intl
