#	$NetBSD: Makefile,v 1.17 2003/02/07 01:52:53 lukem Exp $

MODULE=		groff

ALL_TARGET=	src/include all
MAKE_ARGS=	bindir=${TOOLDIR}/lib/groff

TMACDIR=	${TOOLDIR}/share/groff/tmac

# Add gnuwrap hack directly to the Makefile.
_NOWRAPPER=1

#
# Fixup build/Makefile.
# XXX - the groff distribution Makefiles don't get their dependencies right.
#	We use internal knowledge to specify additional target ordering.
#	This can be removed once our own make(1) can be invoked in true
#	sequential (i.e. compatible) mode again for subdir makes.
#
.configure_done: _post_conf
_post_conf: .USE
	@(echo '.include "${.CURDIR}/../Makefile.gnuwrap"'; \
	  echo '.ORDER: $$(ALLDIRS) dot';\
	  echo '.ORDER: all install' ) >> build/Makefile


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

#
# Since ${TMACDIR} is created by the `realinstall' target, the `afterinstall'
# sources below that depend on it must wait until `realinstall' is done.
#
.for F in andoc doc doc-ditroff doc-common doc-nroff doc-syms doc2html
afterinstall:	${TMACDIR}/tmac.${F}
.PHONY:		${TMACDIR}/tmac.${F}
${TMACDIR}/tmac.${F}: ${F} _installtmac
.ORDER: realinstall ${TMACDIR}/tmac.${F}
.endfor

afterinstall: ${TMACDIR}/tmac.an.old
${TMACDIR}/tmac.an.old: ${GNUHOSTDIST}/tmac/tmac.an _installtmac
.ORDER: realinstall ${TMACDIR}/tmac.an.old

afterinstall: ${TMACDIR}/tmac.an
${TMACDIR}/tmac.an: ${TMACDIR}/tmac.andoc _installtmac
.ORDER: realinstall ${TMACDIR}/tmac.an

# The silly walk below is because sometimes the files begin with "g",
# and sometimes not, depending on what the host tool has at build time.

afterinstall: install.grofflinks
.ORDER: realinstall install.grofflinks
install.grofflinks:
	@cd ${.CURDIR} && ${MAKE} install.grofflinks.recurse

.for F in eqn groff indxbib pic refer soelim tbl
install.grofflinks.recurse: ${TOOLDIR}/bin/${_TOOL_PREFIX}${F}
.if exists(${TOOLDIR}/lib/groff/g${F})
${TOOLDIR}/bin/${_TOOL_PREFIX}${F}: ${TOOLDIR}/lib/groff/g${F}
.else
${TOOLDIR}/bin/${_TOOL_PREFIX}${F}: ${TOOLDIR}/lib/groff/${F}
.endif
	ln -f ${.ALLSRC} ${.TARGET}
.endfor

