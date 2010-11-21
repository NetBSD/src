#	$NetBSD: Makefile,v 1.3 2010/11/21 02:33:40 mrg Exp $

.include <bsd.own.mk>

PROG=	luit
SRCS=	luit.c iso2022.c charset.c parser.c sys.c other.c

CPPFLAGS+=-DLOCALE_ALIAS_FILE=\"${X11LIBDIR}/locale/locale.alias\"
CPPFLAGS+=-DHAVE_CONFIG_H -I${X11SRCDIR.${PROG}}/../include

X11EXTRAMANDEFS+= -e 's,__localealiasfile__,${X11LIBDIR}/locale/locale.alias,g'

LDADD+=	-lfontenc -lz
DPADD+=	${LIBFONTENC} ${LIBZ}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
