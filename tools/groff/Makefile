#	$NetBSD: Makefile,v 1.4 2001/11/14 22:13:40 tv Exp $

MODULE=		groff

ALL_TARGET=	src/include all
MAKE_ARGS=	bindir=${TOOLDIR}/lib/groff

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

.for F in eqn groff indxbib pic refer soelim tbl
afterinstall: ${TOOLDIR}/bin/nb${F}
${TOOLDIR}/bin/nb${F}: ${TOOLDIR}/lib/groff/${F}
	ln -f ${.ALLSRC} ${.TARGET}
.endfor
