#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:06 lukem Exp $

.include <bsd.own.mk>

PROG=	luit
SRCS=	luit.c iso2022.c charset.c parser.c sys.c other.c

CPPFLAGS+=-DLOCALE_ALIAS_FILE=\"${X11LIBDIR}/locale/locale.alias\"

LDADD+=	-lfontenc -lz
DPADD+=	${LIBFONTENC} ${LIBZ}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
