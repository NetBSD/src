#	$NetBSD: Makefile,v 1.1 2001/11/12 23:16:28 tv Exp $

MODULE=		groff

ALL_TARGET=	src/include all

.include "${.CURDIR}/../Makefile.gnuhost"

.PATH:		${.CURDIR}/../../share/tmac
TMACDIR=	${TOOLDIR}/share/groff/tmac

.for F in andoc doc doc-ditroff doc-common doc-nroff doc-syms doc2html
install.files:	${TMACDIR}/tmac.${F}
.PHONY:		${TMACDIR}/tmac.${F}
${TMACDIR}/tmac.${F}: ${F}
.if defined(UPDATE)
	@if ! cmp ${.ALLSRC} ${.TARGET} 2>/dev/null; then \
		echo ${INSTALL_FILE:N-U:Q} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
		${INSTALL_FILE:N-U} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
	fi
.else
	${INSTALL_FILE:N-U} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}
.endif
.endfor

afterinstall: ${TMACDIR}/tmac.an
${TMACDIR}/tmac.an: ${TMACDIR}/tmac.andoc
	${INSTALL_FILE:N-U} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}
