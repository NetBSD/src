#	$NetBSD: Makefile,v 1.16 2003/02/13 09:01:13 wiz Exp $

.include <bsd.own.mk>

SUBDIR=		common .WAIT info infokey install-info makeinfo texi2dvi texindex

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/misc
FILES=		epsf.tex texinfo.dtd texinfo.tex texinfo.xsl
.endif

.if ${MKNLS} != "no"
.for F in cs.gmo da.gmo de.gmo de_AT.gmo eo.gmo fr.gmo he.gmo hr.gmo ja.gmo \
	nl.gmo no.gmo ru.gmo sv.gmo tr.gmo zh_CN.gmo zh_TW.gmo
FILES+=		${F}
FILESDIR_${F}=  /usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}= texinfo.mo
.endfor
# XXX: Do we want to install the following files too?
#FILES+= 	txi-cs.tex txi-de.tex txi-en.tex txi-es.tex txi-fr.tex txi-it.tex \
#		txi-nl.tex txi-no.tex txi-pt.tex txi-tr.tex
.endif

MAN=		info.5 texinfo.5

TEXINFO=	info-stnd.texi info.texi texinfo.txi

DIST=		${NETBSDSRCDIR}/gnu/dist
IDIST=		${DIST}/texinfo
INFOFLAGS=	-I${IDIST}/doc

.PATH: ${IDIST} ${IDIST}/doc ${IDIST}/makeinfo ${IDIST}/po

.include <bsd.info.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
