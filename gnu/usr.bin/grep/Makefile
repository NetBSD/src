#	$NetBSD: Makefile,v 1.20 2001/04/12 11:06:23 pk Exp $

.include <bsd.own.mk>

PROG=		grep
SRCS+=		grep.c dfa.c kwset.c obstack.c savedir.c getopt.c getopt1.c \
		search.c stpcpy.c regex.c grepmat.c
MAN=		grep.1

DIST=		${.CURDIR}/../../dist

CPPFLAGS+=	-DHAVE_CONFIG_H
CLEANFILES+=    ${PROG}.1 ${PROG}.info ${PROG}.cat1

LDADD+=		-lintl
DPADD+=		${LIBINTL}
CPPFLAGS+=	-I${DIST}/grep/src -I${.CURDIR}
CPPFLAGS+=	-DENABLE_NLS -DHAVE_LIBINTL_H=1 \
		-DGNULOCALEDIR=\"/usr/share/locale\" \
		-DLOCALE_ALIAS_PATH=\"/usr/share/locale:.\" \
		-DLOCALEDIR=\"/usr/share/locale\"
# libintl-less build should use this instead of the above lines
#CPPFLAGS+=	-I${DIST}/grep/src -I${.CURDIR}

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
	cp ${DIST}/grep/doc/grep.info .

MLINKS=	grep.1 egrep.1 \
	grep.1 fgrep.1
LINKS=	${BINDIR}/grep ${BINDIR}/egrep \
	${BINDIR}/grep ${BINDIR}/fgrep

.include <bsd.info.mk>
.include <bsd.prog.mk>

.PATH: ${DIST}/grep/doc ${DIST}/grep/intl ${DIST}/grep/po ${DIST}/grep/src 
