#	$NetBSD: Makefile,v 1.11 2002/04/24 08:18:47 lukem Exp $

.include <bsd.own.mk>

SUBDIR=		common .WAIT info install-info makeinfo texi2dvi texindex

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/misc
FILES=		epsf.tex texinfo.tex
.endif

.if ${MKNLS} != "no"
.for F in cs.gmo de.gmo de_AT.gmo eo.gmo fr.gmo nl.gmo no.gmo ru.gmo
FILES+=		${F}
FILESDIR_${F}=  /usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}= texinfo.mo
.endfor
# XXX: Do we want to install the following files too?
#FILES+= 	txi-cs.tex txi-de.tex txi-en.tex txi-es.tex txi-nl.tex \
#		txi-no.tex txi-pt.tex
.endif

MAN=		info.5 texinfo.5

TEXINFO=	info-stnd.texi info.texi texinfo.txi

DIST=		${.CURDIR}/../../dist
INFOFLAGS=	-I${DIST}/texinfo/doc

.PATH: ${DIST}/texinfo ${DIST}/texinfo/doc ${DIST}/texinfo/po

.include <bsd.info.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
