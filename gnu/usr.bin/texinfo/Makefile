#	$NetBSD: Makefile,v 1.1 1999/02/11 15:42:13 tv Exp $

SUBDIR=		common info install-info makeinfo texi2dvi texindex

FILESDIR=	/usr/share/misc
FILES=		epsf.tex texinfo.tex

.for F in de.gmo fr.gmo
FILES+=		${F}
FILESDIR_${F}=  /usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}= texinfo.mo
.endfor

TEXINFO=	info-stnd.texi info.texi texinfo.txi

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/texinfo ${DIST}/texinfo/doc ${DIST}/texinfo/po

all-info all-install-info all-makeinfo all-texi2dvi all-texindex: all-common

.include <bsd.info.mk>
.include <bsd.subdir.mk>
