#	$NetBSD: Makefile,v 1.13 2002/02/01 13:51:38 tv Exp $

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

_installtmac: .USE
.if defined(UPDATE)
	@if ! cmp ${.ALLSRC} ${.TARGET} >/dev/null 2>&1; then \
		echo ${HOST_INSTALL_FILE:Q} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
		${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}; \
	fi
.else
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${.ALLSRC} ${.TARGET}
.endif

.for F in andoc doc doc-ditroff doc-common doc-nroff doc-syms doc2html
afterinstall:	${TMACDIR}/tmac.${F}
.PHONY:		${TMACDIR}/tmac.${F}
${TMACDIR}/tmac.${F}: ${F} _installtmac
.endfor

afterinstall: ${TMACDIR}/tmac.an.old
${TMACDIR}/tmac.an.old: ${DIST}/tmac/tmac.an _installtmac

afterinstall: ${TMACDIR}/tmac.an
${TMACDIR}/tmac.an: ${TMACDIR}/tmac.andoc _installtmac

# The silly walk below is because sometimes the files begin with "g",
# and sometimes not, depending on what the host tool has at build time.

afterinstall: install.grofflinks
install.grofflinks:
	@cd ${.CURDIR} && ${MAKE} install.grofflinks.recurse

.for F in eqn groff indxbib pic refer soelim tbl
install.grofflinks.recurse: ${TOOLDIR}/bin/nb${F}
.if exists(${TOOLDIR}/lib/groff/g${F})
${TOOLDIR}/bin/nb${F}: ${TOOLDIR}/lib/groff/g${F}
.else
${TOOLDIR}/bin/nb${F}: ${TOOLDIR}/lib/groff/${F}
.endif
	ln -f ${.ALLSRC} ${.TARGET}
.endfor

