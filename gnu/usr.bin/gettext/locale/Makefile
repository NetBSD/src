#	$NetBSD: Makefile,v 1.1 2000/11/02 01:09:03 itojun Exp $

.include <bsd.own.mk>

.for F in da.gmo de.gmo es.gmo fr.gmo ko.gmo nl.gmo no.gmo no@nynorsk.gmo \
	pl.gmo pt.gmo sl.gmo sv.gmo
FILES+=		${F}
FILESDIR_${F}=	/usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}=	gettext.mo
.endfor

.include <bsd.prog.mk>

.PATH: ${DIST}/gettext/po
