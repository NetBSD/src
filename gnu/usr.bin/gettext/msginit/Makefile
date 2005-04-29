#	$NetBSD: Makefile,v 1.2 2005/04/29 15:05:01 christos Exp $

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

.PATH: ${DIST}/gettext/gettext-runtime/intl
