#	$NetBSD: bsd.nls.mk,v 1.6 1997/05/06 21:29:42 mycroft Exp $

.if !target(.MAIN)
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN:		all
.endif
.PHONY:		cleannls nlsinstall

.SUFFIXES: .cat .msg

.msg.cat:
	@rm -f ${.TARGET}
	gencat ${.TARGET} ${.IMPSRC}

.if defined(NLS) && !empty(NLS)
NLSALL= ${NLS:.msg=.cat}
.endif

.if !defined(NLSNAME)
.if defined(PROG)
NLSNAME=${PROG}
.else
NLSNAME=lib${LIB}
.endif
.endif

.if defined(NLSALL)
all: ${NLSALL}

cleandir: cleannls
cleannls:
	rm -f ${NLSALL}

.for F in ${NLSALL}
nlsinstall:: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
.endif
.if !defined(BUILD)
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: .MADE
.endif

.PRECIOUS: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: ${F}
	${INSTALL} -d ${.TARGET:H}
	${INSTALL} ${COPY} -o ${NLSOWN} -g ${NLSGRP} -m ${NLSMODE} ${.ALLSRC} \
		${.TARGET}
.endfor
.else
nlsinstall::
.endif

install:  nlsinstall
