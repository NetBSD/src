#	$NetBSD: Makefile,v 1.15 2000/02/27 02:36:55 wiz Exp $

.include <bsd.own.mk>

PROG=		grep
SRCS=		bindtextdom.c dcgettext.c dgettext.c gettext.c finddomain.c \
		loadmsgcat.c localealias.c textdomain.c l10nflist.c \
		explodename.c intl-compat.c
SRCS+=		grep.c dfa.c kwset.c obstack.c savedir.c getopt.c getopt1.c \
		search.c stpcpy.c regex.c grepmat.c
MAN=		grep.1

DIST=		${.CURDIR}/../../dist

CPPFLAGS+=	-I${DIST}/grep/src -I${DIST}/grep/intl -I${.CURDIR} \
		-DHAVE_CONFIG_H \
		-DGNULOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale:.\" \
		-DLOCALEDIR=\"/usr/share/locale\"
CLEANFILES+=    ${PROG}.1 ${PROG}.info ${PROG}.cat1

.if ${MKNLS} != "no"
.for F in de.gmo el.gmo es.gmo fr.gmo ko.gmo nl.gmo no.gmo pl.gmo pt_BR.gmo \
	ru.gmo sl.gmo sv.gmo
FILES+=		${F}
FILESDIR_${F}=	/usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}=	grep.mo
.endfor
.endif

TEXINFO=	grep.texi
INFOFLAGS=	-I${.CURDIR}/../../dist/grep/doc

# this is a hack -- our texinfo doesn't support @command yet, so
# we have to use the pre-generated grep.info until it does
grep.info:
	ln -s ${DIST}/grep/doc/grep.info .

MLINKS=	grep.1 egrep.1 \
	grep.1 fgrep.1
LINKS=	${BINDIR}/grep ${BINDIR}/egrep \
	${BINDIR}/grep ${BINDIR}/fgrep

.include <bsd.info.mk>
.include <bsd.prog.mk>

.PATH: ${DIST}/grep/doc ${DIST}/grep/intl ${DIST}/grep/po ${DIST}/grep/src 
