#	$NetBSD: Makefile,v 1.3 1999/05/31 14:17:44 kleink Exp $

.include <bsd.own.mk>

SUBDIR=		common info install-info makeinfo texi2dvi texindex

FILESDIR=	/usr/share/misc
FILES=		epsf.tex texinfo.tex

.if ${MKNLS} != "no"
.for F in de.gmo fr.gmo
FILES+=		${F}
FILESDIR_${F}=  /usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}= texinfo.mo
.endfor
.endif

TEXINFO=	info-stnd.texi info.texi texinfo.txi

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/texinfo ${DIST}/texinfo/doc ${DIST}/texinfo/po

all-info all-install-info all-makeinfo all-texi2dvi all-texindex: all-common

.include <bsd.info.mk>
.include <bsd.subdir.mk>
