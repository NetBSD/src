#	$NetBSD: Makefile,v 1.3 2001/11/13 14:21:03 tv Exp $

MODULE=		groff

ALL_TARGET=	src/include all

# Add gnuwrap hack directly to the Makefile.
_NOWRAPPER=1
.build_done: build/Makefile
build/Makefile: .configure_done
	@printf '.include "${.CURDIR}/../Makefile.gnuwrap"\n' >>$@

.include "${.CURDIR}/../Makefile.gnuhost"

.PATH:		${.CURDIR}/../../share/tmac

.for F in andoc doc doc-ditroff doc-common doc-nroff doc-syms doc2html
afterinstall:	${TMACDIR}/tmac.${F}
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
