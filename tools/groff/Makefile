#	$NetBSD: Makefile,v 1.8 2001/12/09 23:34:31 jmc Exp $

MODULE=		groff

ALL_TARGET=	src/include all
MAKE_ARGS=	bindir=${TOOLDIR}/lib/groff

TMACDIR=	${TOOLDIR}/share/groff/tmac

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
# Get the hard coded path out of any files and put ${TMACDIR} in it's place
${TMACDIR}/tmac.${F}: ${F}
	@sed -e "s:/usr/share/tmac:${TMACDIR}:g" < ${.ALLSRC} > build/tmac/${F}.sed
.if defined(UPDATE)
	@if ! cmp build/tmac/${F}.sed ${.TARGET} 2>/dev/null; then \
		echo ${HOST_INSTALL_FILE:Q} -m ${NONBINMODE} build/tmac/${F}.sed ${.TARGET}; \
		${HOST_INSTALL_FILE} -m ${NONBINMODE} build/tmac/${F}.sed ${.TARGET}; \
	fi
.else
	${HOST_INSTALL_FILE} -m ${NONBINMODE} build/tmac/${F}.sed ${.TARGET}
.endif
.endfor

afterinstall: ${TMACDIR}/tmac.an.old
${TMACDIR}/tmac.an.old: ${DIST}/tmac/tmac.an
.if defined(UPDATE)
	@if ! cmp ${.ALLSRC} ${.TARGET} 2>/dev/null; then \
		echo ${HOST_INSTALL_FILE:Q} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
		${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
	fi
.else
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}
.endif

afterinstall: ${TMACDIR}/tmac.an
${TMACDIR}/tmac.an: ${TMACDIR}/tmac.andoc
.if defined(UPDATE)
	@if ! cmp ${.ALLSRC} ${.TARGET} 2>/dev/null; then \
		echo ${HOST_INSTALL_FILE:Q} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
		${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
	fi
.else
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}
.endif

.for F in eqn groff indxbib pic refer soelim tbl
afterinstall: ${TOOLDIR}/bin/nb${F}
${TOOLDIR}/bin/nb${F}: ${TOOLDIR}/lib/groff/${F}
	ln -f ${.ALLSRC} ${.TARGET}
.endfor
