#	$NetBSD: Makefile,v 1.3 1999/02/13 02:54:25 lukem Exp $

LIB=		common
SRCS=		bindtextdom.c dcgettext.c dgettext.c gettext.c finddomain.c \
		loadmsgcat.c localealias.c textdomain.c l10nflist.c \
		explodename.c intl-compat.c
SRCS+=		grep.c dfa.c kwset.c obstack.c savedir.c getopt.c getopt1.c \
		search.c stpcpy.c regex.c
CPPFLAGS+=	-DGNULOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale:.\"

MKLINT=		no
MKPIC=		no
MKPROFILE=	no
MKLINKLIB=	no

.include <bsd.lib.mk>

.PATH: ${DIST}/grep/intl
