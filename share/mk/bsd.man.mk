#	$NetBSD: bsd.man.mk,v 1.30 1997/05/07 15:53:32 mycroft Exp $
#	@(#)bsd.man.mk	8.1 (Berkeley) 6/8/93

.if !target(.MAIN)
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN:		all
.endif
.PHONY:		catinstall maninstall catpages manpages catlinks manlinks cleanman
install:	${MANINSTALL}


MANTARGET?=	cat
NROFF?=		nroff

.SUFFIXES: .1 .2 .3 .4 .5 .6 .7 .8 .9 \
	   .cat1 .cat2 .cat3 .cat4 .cat5 .cat6 .cat7 .cat8 .cat9

.9.cat9 .8.cat8 .7.cat7 .6.cat6 .5.cat5 .4.cat4 .3.cat3 .2.cat2 .1.cat1:
	@echo "${NROFF} -mandoc ${.IMPSRC} > ${.TARGET}"
	@${NROFF} -mandoc ${.IMPSRC} > ${.TARGET} || \
	 (rm -f ${.TARGET}; false)

.if defined(MAN) && !empty(MAN)
MANPAGES=	${MAN}
CATPAGES=	${MANPAGES:C/(.*).([1-9])/\1.cat\2/}
.endif

MINSTALL=	${INSTALL} ${COPY} -o ${MANOWN} -g ${MANGRP} -m ${MANMODE}

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
.if defined(CATPAGES) && !empty(CATPAGES)
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
.PHONY: ${DESTDIR}${MANDIR}/${P:T:E}${MANSUBDIR}/${P}${MCOMPRESSSUFFIX}
.	endif

.PRECIOUS: ${DESTDIR}${MANDIR}/man${P:T:E}${MANSUBDIR}/${P}${MCOMPRESSSUFFIX}
${DESTDIR}${MANDIR}/man${P:T:E}${MANSUBDIR}/${P}${MCOMPRESSSUFFIX}: ${P} __installpage
.   endfor
.else
manpages::
.endif

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
		if [ ! -f $$t -o -z "${UPDATE}" ]; then \
		    echo $$t -\> $$l; \
		    rm -f $$t; \
		    ln $$l $$t; \
		fi; \
	done
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
		if [ ! -f $$t -o -z "${UPDATE}" ]; then \
		    echo $$t -\> $$l; \
		    rm -f $$t; \
		    ln $$l $$t; \
		fi; \
	done
.endif
.if defined(CATPAGES)
all: ${CATPAGES}

cleandir: cleanman
cleanman:
	rm -f ${CATPAGES}
.endif
