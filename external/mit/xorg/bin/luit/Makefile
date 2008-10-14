#	$NetBSD: Makefile,v 1.2 2008/10/14 23:37:19 cube Exp $

.include <bsd.own.mk>

PROG=	luit
SRCS=	luit.c iso2022.c charset.c parser.c sys.c other.c

CPPFLAGS+=-DLOCALE_ALIAS_FILE=\"${X11LIBDIR}/locale/locale.alias\"

X11EXTRAMANDEFS+= -e 's,__localealiasfile__,${X11LIBDIR}/locale/locale.alias,g'

LDADD+=	-lfontenc -lz
DPADD+=	${LIBFONTENC} ${LIBZ}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
