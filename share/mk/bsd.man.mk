#	$NetBSD: bsd.man.mk,v 1.76 2002/02/07 00:52:23 ross Exp $
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
CATDEPS?=	${TMACDEPDIR}/tmac.andoc \
		${TMACDEPDIR}/tmac.doc \
		${TMACDEPDIR}/tmac.doc-ditroff \
		${TMACDEPDIR}/tmac.doc-common \
		${TMACDEPDIR}/tmac.doc-nroff \
		${TMACDEPDIR}/tmac.doc-syms
HTMLDEPS?=	${TMACDEPDIR}/tmac.doc2html
MANTARGET?=	cat

GROFF_HTML?=	${GROFF} -Tlatin1 -mdoc2html -P-b -P-o -P-u
NROFF?=		${GROFF} -Tascii -mtty-char
TBL?=		tbl

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
		${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL_FILE} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE} \
		${.ALLSRC} ${.TARGET})

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

${_F}:		${F}${MANSUFFIX} __installpage		# install rule
manpages::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
#.PHONY:		${UPDATE:D:U${_F}}			# clobber unless UPDATE
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
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
			${INSTALL_LINK} $$l $$t; \
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
	${TBL} ${.IMPSRC} | ${NROFF} -mandoc ${MANCOMPRESS} > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}
.else
	${NROFF} -mandoc ${.IMPSRC} ${MANCOMPRESS} > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}
.endif

.for F in ${CATPAGES:S/${MANSUFFIX}$//:O:u}
_F:=		${DESTDIR}${MANDIR}/${F:T:E}${MANSUBDIR}/${F:R}.0${MANSUFFIX}
${_F}:		${F}${MANSUFFIX} __installpage		# install rule
catpages::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
#.PHONY:		${UPDATE:D:U${_F}}			# noclobber install
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
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
			${INSTALL_LINK} $$l $$t; \
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
	${GROFF_HTML} ${.IMPSRC} > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}

.for F in ${HTMLPAGES:O:u}
_F:=		${HTMLDIR}/${F:T:E}/${F:R}.html		# installed path
${_F}:		${F} __installpage			# install rule
htmlpages::	${_F}
.PRECIOUS:	${_F}					# keep if install fails
#.PHONY:		${UPDATE:D:U${_F}}			# noclobber install
.if !defined(BUILD) && !make(all) && !make(${F})
${_F}:		.MADE					# no build at install
.endif
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
