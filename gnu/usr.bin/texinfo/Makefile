#	$NetBSD: Makefile,v 1.8 2001/09/02 22:16:58 wiz Exp $

.include <bsd.own.mk>

SUBDIR=		common .WAIT info install-info makeinfo texi2dvi texindex

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/misc
FILES=		epsf.tex texinfo.tex
.endif

.if ${MKNLS} != "no"
.for F in de.gmo fr.gmo
FILES+=		${F}
FILESDIR_${F}=  /usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}= texinfo.mo
.endfor
.endif

MAN=		info.5 texinfo.5

TEXINFO=	info-stnd.texi info.texi texinfo.txi

DIST=		${.CURDIR}/../../dist
INFOFLAGS=	-I${DIST}/texinfo/doc

.PATH: ${DIST}/texinfo ${DIST}/texinfo/doc ${DIST}/texinfo/po

.include <bsd.man.mk>
.include <bsd.info.mk>
.include <bsd.files.mk>
.include <bsd.subdir.mk>
