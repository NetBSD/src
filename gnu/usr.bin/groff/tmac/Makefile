# $NetBSD: Makefile,v 1.15 2003/06/30 18:47:57 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/tmac/Makefile,v 1.41 2003/05/01 13:22:21 ru Exp $

TMACOWN?=	${BINOWN}
TMACGRP?=	${BINGRP}
TMACMODE?=	${NOBINMODE}
# XXX: next depends on ../src/include/defs.h
TMACDIR?=	/usr/share/tmac
MDOCDIR?=	${TMACDIR}/mdoc

MAN=		groff_ms.7 groff_man.7 groff_me.7 groff_mdoc.7
MAN+=		groff_trace.7 groff_www.7
MLINKS=		groff_ms.7 ms.7
MLINKS+=	groff_man.7 man.7
MLINKS+=	groff_me.7 me.7
MLINKS+=	groff_mdoc.7 mdoc.7 groff_mdoc.7 mdoc.samples.7

CLEANFILES=	${MAN} ${MDOCFILES:S/$/-s/} ${STRIPFILES:S/$/-s/} ${SPECIALFILES:S/$/-s/}

# XXX: hyphen.ru
NORMALFILES=	mandoc.tmac andoc.tmac an-old.tmac \
		me.tmac \
		mdoc.tmac \
		pic.tmac \
		a4.tmac \
		papersize.tmac \
		ec.tmac \
		safer.tmac \
		trace.tmac \
		ps.tmac psold.tmac pspic.tmac psatk.tmac \
		dvi.tmac \
		tty.tmac tty-char.tmac \
		latin1.tmac latin2.tmac latin9.tmac cp1047.tmac \
		X.tmac Xps.tmac \
		lj4.tmac \
		lbp.tmac \
		html.tmac html-end.tmac \
		europs.tmac \
		composite.tmac \
		eqnrc \
		troffrc troffrc-end \
		hyphen.us hyphenex.us
SPECIALFILES=	an.tmac man.tmac s.tmac ms.tmac www.tmac
STRIPFILES=	e.tmac doc.tmac man.local mdoc.local
MDOCFILES=	doc-common doc-ditroff doc-nroff doc-syms \
		fr.ISO8859-1 ru.KOI8-R

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	${TMACDIR}
dependall: ${MDOCFILES:S/$/-s/} ${STRIPFILES:S/$/-s/} ${SPECIALFILES:S/$/-s/}
install: ${MDOCFILES:S/$/-s/} ${STRIPFILES:S/$/-s/} ${SPECIALFILES:S/$/-s/}

.for f in ${MDOCFILES}
FILES+= $f-s
FILESNAME_$f-s=mdoc/$f
$f-s: $f
	sed -f ${DIST_DIR}/strip.sed ${.ALLSRC} > ${.TARGET}
.endfor

.for f in ${STRIPFILES}
FILES+= $f-s
FILESNAME_$f-s=$f
$f-s: $f
	sed -f ${DIST_DIR}/strip.sed ${.ALLSRC} > ${.TARGET}
.endfor

.for f in ${SPECIALFILES}
FILES+= $f-s
FILESNAME_$f-s=$f
$f-s: $f
	sed -e "s;@TMAC_AN_PREFIX@;${tmac_an_prefix};g" \
	    -e "s;@TMAC_S_PREFIX@;${tmac_s_prefix};g" \
	    -e "s;@PNMTOPS_NOSETPAGE@;pnmtops -nosetpage;g" \
		${.ALLSRC} > ${.TARGET}
.endfor

FILES+=	${NORMALFILES}

donotbeforeinstall:
.for f in ${STRIPFILES} ${SPECIALFILES}
	${INSTALL} -c -o ${TMACOWN} -g ${TMACGRP} -m ${TMACMODE} \
	    $f-s ${DESTDIR}${TMACDIR}/$f
.endfor
.for f in ${MDOCFILES}
	${INSTALL} -c -o ${TMACOWN} -g ${TMACGRP} -m ${TMACMODE} \
	    $f-s ${DESTDIR}${MDOCDIR}/$f
.endfor
.if !exists(${DESTDIR}${TMACDIR}/man.local)
	${INSTALL} -c -o ${TMACOWN} -g ${TMACGRP} -m ${TMACMODE} \
	    ${DIST_DIR}/man.local ${DESTDIR}${TMACDIR}
.endif
.endif

.include <bsd.prog.mk>
