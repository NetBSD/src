#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:28 lukem Exp $

LIB=		common
SRCS=		bindtextdom.c dcgettext.c dgettext.c gettext.c finddomain.c \
		loadmsgcat.c localealias.c textdomain.c l10nflist.c \
		explodename.c intl-compat.c
SRCS+=		getopt.c getopt1.c xmalloc.c xstrdup.c

CPPFLAGS+=	-DGNULOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale:.\"

MKLINT=		no
MKPROFILE=	no
MKPIC=		no
MKLINKLIB=	no

.include <bsd.lib.mk>

.PATH: ${DIST}/texinfo/intl ${DIST}/texinfo/lib ${DIST}/texinfo/po

all: ${FILES}
.SUFFIXES: .gmo .mo
.gmo.mo:
	@cp $< $@
