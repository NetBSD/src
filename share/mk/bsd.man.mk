#	$NetBSD: bsd.man.mk,v 1.81 2003/07/10 10:34:36 lukem Exp $
#	@(#)bsd.man.mk	8.1 (Berkeley) 6/8/93

.include <bsd.init.mk>

##### Basic targets
.PHONY:		catinstall maninstall catpages manpages catlinks manlinks \
		cleanman html installhtml cleanhtml
realinstall:	${MANINSTALL}

##### Default values
.if ${USETOOLS} == "yes"
TMACDEPDIR?=	${TOOLDIR}/share/groff/tmac
.else
TMACDEPDIR?=	${DESTDIR}/usr/share/tmac
.endif

HTMLDIR?=	${DESTDIR}/usr/share/man
CATDEPS?=	${TMACDEPDIR}/andoc.tmac \
		${TMACDEPDIR}/doc.tmac \
		${TMACDEPDIR}/mdoc/doc-common \
		${TMACDEPDIR}/mdoc/doc-ditroff \
		${TMACDEPDIR}/mdoc/doc-nroff \
		${TMACDEPDIR}/mdoc/doc-syms
HTMLDEPS?=	${TMACDEPDIR}/doc2html.tmac
MANTARGET?=	cat

MAN?=
MLINKS?=
_MNUMBERS=	1 2 3 4 5 6 7 8 9
.SUFFIXES:	${_MNUMBERS:@N@.$N@}

MANCOMPRESS?=	${MANZ:Dgzip -cf}
MANSUFFIX?=	${MANZ:D.gz}

# make MANCOMPRESS a filter, so it can be inserted on an as-needed basis
.if !empty(MANCOMPRESS)
MANCOMPRESS:=	| ${MANCOMPRESS}
.endif

__installpage: .USE
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL_FILE} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} \
		${SYSPKGDOCTAG} ${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL_FILE} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} \
		${SYSPKGDOCTAG} ${.ALLSRC} ${.TARGET})

##### Build and install rules (source form pages)

.if ${MKMAN} != "no"
maninstall:	manlinks
manpages::	# ensure target exists
MANPAGES=	${MAN:C/.$/&${MANSUFFIX}/}

realall:	${MANPAGES}
.if !empty(MANSUFFIX)
.NOPATH:	${MANPAGES}
.SUFFIXES:	${_MNUMBERS:@N@.$N${MANSUFFIX}@}

${_MNUMBERS:@N@.$N.$N${MANSUFFIX}@}:			# build rule
	cat ${.IMPSRC} ${MANCOMPRESS} > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}
.endif # !empty(MANSUFFIX)

.for F in ${MANPAGES:S/${MANSUFFIX}$//:O:u}
_F:=		${DESTDIR}${MANDIR}/man${F:T:E}${MANSUBDIR}/${F}${MANSUFFIX}

.if !defined(UPDATE)
${_F}!		${F}${MANSUFFIX} __installpage		# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}!		.MADE					# no build at install
.endif
.else
${_F}:		${F}${MANSUFFIX} __installpage		# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
.endif

manpages::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
.endfor

manlinks: manpages					# symlink install
.if !empty(MLINKS)
	@set ${MLINKS}; \
	while test $$# -ge 2; do \
		name=$$1; shift; \
		dir=${DESTDIR}${MANDIR}/man$${name##*.}; \
		l=$${dir}${MANSUBDIR}/$${name}${MANSUFFIX}; \
		name=$$1; shift; \
		dir=${DESTDIR}${MANDIR}/man$${name##*.}; \
		t=$${dir}${MANSUBDIR}/$${name}${MANSUFFIX}; \
		if test $$l -nt $$t -o ! -f $$t; then \
			echo $$t -\> $$l; \
			${INSTALL_LINK} ${SYSPKGDOCTAG} $$l $$t; \
		fi; \
	done
.endif
.endif # ${MKMAN} != "no"

##### Build and install rules (plaintext pages)

.if (${MKCATPAGES} != "no") && (${MKMAN} != "no")
catinstall:	catlinks
catpages::	# ensure target exists
CATPAGES=	${MAN:C/\.([1-9])$/.cat\1${MANSUFFIX}/}

realall:	${CATPAGES}
.NOPATH:	${CATPAGES}
.SUFFIXES:	${_MNUMBERS:@N@.cat$N${MANSUFFIX}@}
.MADE:	${CATDEPS}
.MADE:	${HTMLDEPS}

${_MNUMBERS:@N@.$N.cat$N${MANSUFFIX}@}: ${CATDEPS}	# build rule
.if defined(USETBL)
	${TOOL_TBL} ${.IMPSRC} | ${TOOL_ROFF_ASCII} -mandoc ${MANCOMPRESS} \
	    > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}
.else
	${TOOL_ROFF_ASCII} -mandoc ${.IMPSRC} ${MANCOMPRESS} \
	    > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}
.endif

.for F in ${CATPAGES:S/${MANSUFFIX}$//:O:u}
_F:=		${DESTDIR}${MANDIR}/${F:T:E}${MANSUBDIR}/${F:R}.0${MANSUFFIX}

.if !defined(UPDATE)
${_F}!		${F}${MANSUFFIX} __installpage		# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}!		.MADE					# no build at install
.endif
.else
${_F}:		${F}${MANSUFFIX} __installpage		# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
.endif

catpages::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
.endfor

catlinks: catpages					# symlink install
.if !empty(MLINKS)
	@set ${MLINKS}; \
	while test $$# -ge 2; do \
		name=$$1; shift; \
		dir=${DESTDIR}${MANDIR}/cat$${name##*.}; \
		l=$${dir}${MANSUBDIR}/$${name%.*}.0${MANSUFFIX}; \
		name=$$1; shift; \
		dir=${DESTDIR}${MANDIR}/cat$${name##*.}; \
		t=$${dir}${MANSUBDIR}/$${name%.*}.0${MANSUFFIX}; \
		if test $$l -nt $$t -o ! -f $$t; then \
			echo $$t -\> $$l; \
			${INSTALL_LINK} ${SYSPKGDOCTAG} $$l $$t; \
		fi; \
	done
.endif
.endif # (${MKCATPAGES} != "no") && (${MKMAN} != "no")

##### Build and install rules (HTML pages)

.if !defined(NOHTML)
installhtml:	htmlpages
htmlpages::	# ensure target exists
HTMLPAGES=	${MAN:C/\.([1-9])$/.html\1/}

html:		${HTMLPAGES}
.NOPATH:	${HTMLPAGES}
.SUFFIXES:	${_MNUMBERS:@N@.html$N@}

${_MNUMBERS:@N@.$N.html$N@}: ${HTMLDEPS}			# build rule
	${TOOL_ROFF_HTML} ${.IMPSRC} > ${.TARGET}.tmp && \
	    mv ${.TARGET}.tmp ${.TARGET}

.for F in ${HTMLPAGES:O:u}
# construct installed path
_F:=		${HTMLDIR}/${F:T:E}${MANSUBDIR}/${F:R:S-/index$-/x&-}.html

.if !defined(UPDATE)
${_F}!		${F} __installpage			# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}!		.MADE					# no build at install
.endif
.else
${_F}:		${F} __installpage			# install rule
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
.endif

htmlpages::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
.endfor

cleanhtml:
	rm -f ${HTMLPAGES}
.endif # !defined(NOHTML)

##### Clean rules
.undef _F

cleandir: cleanman
cleanman:
.if !empty(MAN) && (${MKMAN} != "no")
.if (${MKCATPAGES} != "no")
	rm -f ${CATPAGES}
.endif
.if !empty(MANSUFFIX)
	rm -f ${MANPAGES} ${CATPAGES:S/${MANSUFFIX}$//}
.endif
.endif
# (XXX ${CATPAGES:S...} cleans up old .catN files where .catN.gz now used)

##### Pull in related .mk logic
.include <bsd.obj.mk>
.include <bsd.sys.mk>

${TARGETS} catinstall maninstall: # ensure existence
