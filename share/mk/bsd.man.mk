#	$NetBSD: bsd.man.mk,v 1.44 1999/02/12 12:38:45 lukem Exp $
#	@(#)bsd.man.mk	8.1 (Berkeley) 6/8/93

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.MAIN:		all
.endif

.PHONY:		catinstall maninstall catpages manpages catlinks manlinks cleanman
.if ${MKMAN} != "no"
realinstall:	${MANINSTALL}
.endif
cleandir distclean: cleanman

TMACDIR?=	${DESTDIR}/usr/share/tmac
CATDEPS?=	${TMACDIR}/tmac.andoc \
		${TMACDIR}/tmac.doc \
		${TMACDIR}/tmac.doc-ditroff \
		${TMACDIR}/tmac.doc-common \
		${TMACDIR}/tmac.doc-nroff \
		${TMACDIR}/tmac.doc-syms
MANTARGET?=	cat
NROFF?=		nroff
TBL?=		tbl

.SUFFIXES: .1 .2 .3 .4 .5 .6 .7 .8 .9 \
	   .cat1 .cat2 .cat3 .cat4 .cat5 .cat6 .cat7 .cat8 .cat9

.9.cat9 .8.cat8 .7.cat7 .6.cat6 .5.cat5 .4.cat4 .3.cat3 .2.cat2 .1.cat1: \
    ${CATDEPS}
.if !defined(USETBL)
	@echo "${NROFF} -mandoc ${.IMPSRC} > ${.TARGET}"
	@${NROFF} -mandoc ${.IMPSRC} > ${.TARGET} || \
	 (rm -f ${.TARGET}; false)
.else
	@echo "${TBL} ${.IMPSRC} | ${NROFF} -mandoc > ${.TARGET}"
	@${TBL} ${.IMPSRC} | ${NROFF} -mandoc > ${.TARGET} || \
	 (rm -f ${.TARGET}; false)
.endif

.if defined(MAN) && !empty(MAN)
MANPAGES=	${MAN}
CATPAGES=	${MANPAGES:C/(.*).([1-9])/\1.cat\2/}
.endif

MINSTALL=	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -o ${MANOWN} \
			-g ${MANGRP} -m ${MANMODE}

.if defined(MANZ)
# chown and chmod are done afterward automatically
MCOMPRESS=	gzip -cf
MCOMPRESSSUFFIX= .gz
.endif

catinstall: catlinks
maninstall: manlinks

__installpage: .USE
.if defined(MCOMPRESS) && !empty(MCOMPRESS)
	@rm -f ${.TARGET}
	${MCOMPRESS} ${.ALLSRC} > ${.TARGET}
	@chown ${MANOWN}:${MANGRP} ${.TARGET}
	@chmod ${MANMODE} ${.TARGET}
.else
	${MINSTALL} ${.ALLSRC} ${.TARGET}
.endif


# Rules for cat'ed man page installation
.if defined(CATPAGES) && !empty(CATPAGES) && ${MKCATPAGES} != "no"
.   for P in ${CATPAGES}
catpages:: ${DESTDIR}${MANDIR}/${P:T:E}${MANSUBDIR}/${P:T:R}.0${MCOMPRESSSUFFIX}

.	if !defined(UPDATE)
.PHONY: ${DESTDIR}${MANDIR}/${P:T:E}${MANSUBDIR}/${P:T:R}.0${MCOMPRESSSUFFIX}
.	endif
.	if !defined(BUILD)
${DESTDIR}${MANDIR}/${P:T:E}${MANSUBDIR}/${P:T:R}.0${MCOMPRESSSUFFIX}: .MADE
.	endif

.PRECIOUS: ${DESTDIR}${MANDIR}/${P:T:E}${MANSUBDIR}/${P:T:R}.0${MCOMPRESSSUFFIX}
${DESTDIR}${MANDIR}/${P:T:E}${MANSUBDIR}/${P:T:R}.0${MCOMPRESSSUFFIX}: ${P} __installpage
.   endfor
.else
catpages::
.endif

# Rules for source page installation
.if defined(MANPAGES) && !empty(MANPAGES)
.   for P in ${MANPAGES}
manpages:: ${DESTDIR}${MANDIR}/man${P:T:E}${MANSUBDIR}/${P}${MCOMPRESSSUFFIX}
.	if !defined(UPDATE)
.PHONY: ${DESTDIR}${MANDIR}/man${P:T:E}${MANSUBDIR}/${P}${MCOMPRESSSUFFIX}
.	endif

.PRECIOUS: ${DESTDIR}${MANDIR}/man${P:T:E}${MANSUBDIR}/${P}${MCOMPRESSSUFFIX}
${DESTDIR}${MANDIR}/man${P:T:E}${MANSUBDIR}/${P}${MCOMPRESSSUFFIX}: ${P} __installpage
.   endfor
.else
manpages::
.endif

.if ${MKCATPAGES} != "no"
catlinks: catpages
.if defined(MLINKS) && !empty(MLINKS)
	@set ${MLINKS}; \
	while test $$# -ge 2; do \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}/cat$${name##*.}; \
		l=$${dir}${MANSUBDIR}/$${name%.*}.0${MCOMPRESSSUFFIX}; \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}/cat$${name##*.}; \
		t=$${dir}${MANSUBDIR}/$${name%.*}.0${MCOMPRESSSUFFIX}; \
		if [ -z "${UPDATE}" -o ! -f $$t ]; then \
		    echo $$t -\> $$l; \
		    ln -f $$l $$t; \
		fi; \
	done
.endif
.else
catlinks:
.endif

manlinks: manpages
.if defined(MLINKS) && !empty(MLINKS)
	@set ${MLINKS}; \
	while test $$# -ge 2; do \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}/man$${name##*.}; \
		l=$${dir}${MANSUBDIR}/$${name}${MCOMPRESSSUFFIX}; \
		name=$$1; \
		shift; \
		dir=${DESTDIR}${MANDIR}/man$${name##*.}; \
		t=$${dir}${MANSUBDIR}/$${name}${MCOMPRESSSUFFIX}; \
		if [ -z "${UPDATE}" -o ! -f $$t ]; then \
		    echo $$t -\> $$l; \
		    ln -f $$l $$t; \
		fi; \
	done
.endif

.if defined(CATPAGES)
.if ${MKCATPAGES} != "no"
all: ${CATPAGES}
.else
all:
.endif

cleanman:
	rm -f ${CATPAGES}
.else
cleanman:
.endif

# Make sure all of the standard targets are defined, even if they do nothing.
clean depend includes lint regress tags:
