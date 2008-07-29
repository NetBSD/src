#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	luit
SRCS=	luit.c iso2022.c charset.c parser.c sys.c other.c

CPPFLAGS+=-DLOCALE_ALIAS_FILE=\"${X11LIBDIR}/locale/locale.alias\"

LDADD+=	-lfontenc -lz
DPADD+=	${LIBFONTENC} ${LIBZ}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
