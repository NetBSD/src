#	$NetBSD: Makefile,v 1.10 1999/02/11 17:26:22 tv Exp $

SUBDIR=		common egrep fgrep grep

all-egrep all-fgrep all-grep: all-common

.for F in de.gmo el.gmo es.gmo fr.gmo ko.gmo nl.gmo no.gmo pl.gmo ru.gmo \
	sl.gmo sv.gmo
FILES+=		${F}
FILESDIR_${F}=	/usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}=	grep.mo
.endfor

TEXINFO=	grep.texi
INFOFLAGS=	-I${.CURDIR}/../../dist/grep/doc
.PATH: ${.CURDIR}/../../dist/grep/doc ${.CURDIR}/../../dist/grep/po

.include <bsd.info.mk>
.include <bsd.subdir.mk>
