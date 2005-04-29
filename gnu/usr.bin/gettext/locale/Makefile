#	$NetBSD: Makefile,v 1.2 2005/04/29 15:05:01 christos Exp $

.include <bsd.own.mk>

GMOFILES= be.gmo ca.gmo cs.gmo da.gmo de.gmo el.gmo en@boldquot.gmo \
    en@quot.gmo es.gmo et.gmo fi.gmo fr.gmo gl.gmo id.gmo it.gmo \
    ja.gmo ko.gmo nl.gmo nn.gmo no.gmo pl.gmo pt.gmo pt_BR.gmo ro.gmo \
    ru.gmo sk.gmo sl.gmo sr.gmo sv.gmo tr.gmo uk.gmo zh_CN.gmo zh_TW.gmo 

.for F in ${GMOFILES}
FILES+=		${F}
FILESDIR_${F}=	/usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}=	gettext-tools.mo
.endfor

.include <bsd.prog.mk>

.PATH: ${DIST}/gettext/gettext-tools/po
