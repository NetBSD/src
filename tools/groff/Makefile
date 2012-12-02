#	$NetBSD: Makefile,v 1.30 2012/12/02 12:48:32 apb Exp $

MODULE=		groff

ALL_TARGET=	src/include all
MAKE_ARGS=	bindir=${TOOLDIR}/lib/groff MAKEINFO=${TOOL_MAKEINFO:Q} \
		INSTALL_INFO=${TOOL_INSTALL_INFO:Q}
# we don't need any of the X applications
CONFIGURE_ARGS+=	--without-x

TMACDIR=	${TOOLDIR}/share/groff/tmac
SITETMACDIR=	${TOOLDIR}/share/groff/site-tmac

# Add gnuwrap hack directly to the Makefile.
_NOWRAPPER=1

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
.-include "${TOOLDIR}/share/compat/defs.mk"

.PATH:	${.CURDIR}/../../share/tmac ${.CURDIR}/../../gnu/usr.bin/groff/tmac

_installtmac: .USE
.if ${MKUPDATE} != "no"
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
#.for F in an andoc doc doc-ditroff doc-common doc-nroff doc-syms html html-end man man.local mdoc mdoc.local
.for F in doc2html
afterinstall:	${TMACDIR}/${F}.tmac
.PHONY:		${TMACDIR}/${F}.tmac
${TMACDIR}/${F}.tmac: ${F} _installtmac
.ORDER: realinstall ${TMACDIR}/${F}.tmac
.endfor
.for F in man.local mdoc.local
afterinstall:	${SITETMACDIR}/${F}
.PHONY:		${SITETMACDIR}/${F}
${SITETMACDIR}/${F}: ${F}-s _installtmac
.ORDER: realinstall ${SITETMACDIR}/${F}
CLEANFILES+=${F}-s
${F}-s: ${F}
	${TOOL_SED} -f ${.CURDIR}/../../gnu/dist/groff/tmac/strip.sed \
		${.ALLSRC} > ${.TARGET}
.endfor

# The silly walk below is because sometimes the files begin with "g",
# and sometimes not, depending on what the host tool has at build time.

afterinstall: install.grofflinks
.ORDER: realinstall install.grofflinks
install.grofflinks:
	@cd ${.CURDIR} && ${MAKE} install.grofflinks.recurse

.for F in eqn groff indxbib pic refer soelim tbl
install.grofflinks.recurse: ${TOOLDIR}/bin/${_TOOL_PREFIX}${F}${HOSTEXEEXT}
.if exists(${TOOLDIR}/lib/groff/g${F}${HOSTEXEEXT})
${TOOLDIR}/bin/${_TOOL_PREFIX}${F}${HOSTEXEEXT}: ${TOOLDIR}/lib/groff/g${F}${HOSTEXEEXT}
.else
${TOOLDIR}/bin/${_TOOL_PREFIX}${F}${HOSTEXEEXT}: ${TOOLDIR}/lib/groff/${F}${HOSTEXEEXT}
.endif
	ln -f ${.ALLSRC} ${.TARGET}
.endfor

.for F in nroff
install.grofflinks.recurse: ${TOOLDIR}/bin/${_TOOL_PREFIX}${F}
.if exists(${TOOLDIR}/lib/groff/g${F})
${TOOLDIR}/bin/${_TOOL_PREFIX}${F}: ${TOOLDIR}/lib/groff/g${F}
.else
${TOOLDIR}/bin/${_TOOL_PREFIX}${F}: ${TOOLDIR}/lib/groff/${F}
.endif
	ln -f ${.ALLSRC} ${.TARGET}
.endfor
